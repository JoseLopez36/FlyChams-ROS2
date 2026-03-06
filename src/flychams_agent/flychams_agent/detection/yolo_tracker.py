"""
YOLO-based target tracker implementation for the FlyChams system
Integrates YOLO detection with KLT GPU tracking and ByteTrack/BOT-SORT
Author: Alfonso Martínez Sánchez
Date: 2026-03-02
"""

import os
import time
from typing import Any, Dict

from ament_index_python.packages import get_package_share_directory

import numpy as np
import cv2
from ultralytics import YOLO
from ultralytics.utils import YAML, IterableSimpleNamespace
from ultralytics.utils.checks import check_yaml
from ultralytics.trackers.byte_tracker import BYTETracker
from ultralytics.trackers.bot_sort import BOTSORT
from ultralytics.engine.results import Boxes

# Constants
DEFAULT_CONF_TH = 0.20
DEFAULT_IMGSZ = 640
DEFAULT_TRACKER_YAML = "bytetrack.yaml" # or "botsort.yaml"
DEFAULT_TRACK_BUFFER_CYCLES = 10
DEFAULT_TARGET_CLASS = "person"

# KLT GPU Constants
MAX_PTS = 40
MIN_PTS = 8
LK_WIN = (21, 21)
LK_MAX_LEVEL = 2

class KLT_GPU:
    """KLT tracking using OpenCV CUDA"""
    
    def __init__(self, logger=None):
        self.logger = logger
        if cv2.cuda.getCudaEnabledDeviceCount() == 0:
            raise RuntimeError("OpenCV does not have CUDA support compiled")

        self.detector = cv2.cuda.createGoodFeaturesToTrackDetector(
            srcType=cv2.CV_8UC1,
            maxCorners=MAX_PTS,
            qualityLevel=0.01,
            minDistance=5,
            blockSize=7
        )

        self.lk = cv2.cuda.SparsePyrLKOpticalFlow_create(
            winSize=LK_WIN,
            maxLevel=LK_MAX_LEVEL,
            iters=30
        )

    def init_points(self, gpu_gray, bbox_xyxy):
        """Initialize KLT points within a bounding box"""
        cols, rows = gpu_gray.size()
        cols, rows = int(cols), int(rows)

        x1, y1, x2, y2 = bbox_xyxy
        ix1 = max(0, int(np.floor(x1)))
        iy1 = max(0, int(np.floor(y1)))
        ix2 = min(cols, int(np.ceil(x2)))
        iy2 = min(rows, int(np.ceil(y2)))

        if (ix2 - ix1) < 2 or (iy2 - iy1) < 2:
            return None

        try:
            roi_gpu = gpu_gray.colRange(ix1, ix2).rowRange(iy1, iy2)
        except cv2.error:
            return None

        if roi_gpu.empty():
            return None

        pts_gpu = self.detector.detect(roi_gpu)
        if pts_gpu is None or pts_gpu.empty():
            return None

        pts_cpu = pts_gpu.download()
        if pts_cpu is None or len(pts_cpu) == 0:
            return None

        pts_cpu[:, 0, 0] += ix1
        pts_cpu[:, 0, 1] += iy1

        final_pts_gpu = cv2.cuda_GpuMat()
        final_pts_gpu.upload(pts_cpu)
        return final_pts_gpu

    def update(self, prev_gpu_gray, curr_gpu_gray, pts_gpu):
        """Update KLT points and return median displacement"""
        if pts_gpu is None or pts_gpu.empty():
            return None, None, False

        new_pts_gpu, status_gpu, _ = self.lk.calc(prev_gpu_gray, curr_gpu_gray, pts_gpu, None)

        pts_cpu = pts_gpu.download()
        new_pts_cpu = new_pts_gpu.download()
        status_cpu = status_gpu.download()
        
        if status_cpu is None:
            return None, None, False

        status_flat = status_cpu.reshape(-1)
        good_old = pts_cpu[status_flat == 1]
        good_new = new_pts_cpu[status_flat == 1]

        if len(good_new) < MIN_PTS:
            return None, None, False

        d = (good_new - good_old).reshape(-1, 2)
        dx, dy = np.median(d[:, 0]), np.median(d[:, 1])

        valid_pts_gpu = cv2.cuda_GpuMat()
        valid_pts_gpu.upload(good_new.reshape(-1, 1, 2))
        return valid_pts_gpu, (float(dx), float(dy)), True

class YoloTracker():
    """YOLO-based tracker implementation"""

    def __init__(self, 
                 model_name: str = "yolov11n.pt", 
                 target_class: str = DEFAULT_TARGET_CLASS,
                 conf_th: float = DEFAULT_CONF_TH,
                 imgsz: int = DEFAULT_IMGSZ,
                 tracker_yaml: str = DEFAULT_TRACKER_YAML,
                 source: str = "udp://127.0.0.1:6000",
                 logger=None):
        """
        Initialize the YOLO tracker
        
        Args:
            model_name: Name of the YOLO model (e.g., yolov11n)
            target_class: Name of the class to track
            conf_th: Confidence threshold for detection
            imgsz: Image size for YOLO inference
            tracker_yaml: YAML configuration for the tracker (ByteTrack/BOT-SORT)
            source: Video stream source (e.g. UDP port)
            logger: Logger instance
        """
        self.logger = logger
        
        self.target_class = target_class
        self.conf_th = conf_th
        self.imgsz = imgsz

        # Resolve model path
        pkg_share = get_package_share_directory('flychams_agent')
        model_path = os.path.join(pkg_share, "models", model_name)

        if self.logger:
            self.logger.info(f"Loading YOLO model from: {model_path}")

        # Initialize YOLO model
        self.model = YOLO(model_path, task="detect")
        
        # Check for CUDA availability
        import torch
        if torch.cuda.is_available():
            self.device = 0
            if self.logger:
                self.logger.info("CUDA is available. Using GPU (device 0)")
        else:
            self.device = 'cpu'
            if self.logger:
                self.logger.warning("CUDA is NOT available. Falling back to CPU. This will be slow!")

        self.target_cls_id = self._get_target_cls_id(self.model.names, self.target_class)
        
        if self.target_cls_id is None:
            if self.logger:
                self.logger.error(f"Target class '{self.target_class}' not found in model names")

        # Initialize KLT GPU
        try:
            self.klt_gpu = KLT_GPU(logger=self.logger)
        except Exception as e:
            if self.logger:
                self.logger.error(f"Failed to initialize KLT GPU: {e}")
            self.klt_gpu = None

        # Initialize Ultralytics tracker (ByteTrack or BOT-SORT)
        tracker_cfg_path = check_yaml(tracker_yaml)
        cfg = IterableSimpleNamespace(**YAML.load(tracker_cfg_path))
        cfg.track_buffer = int(max(cfg.track_buffer, DEFAULT_TRACK_BUFFER_CYCLES))
        
        if cfg.tracker_type == "botsort":
            self.ultralytics_tracker = BOTSORT(args=cfg, frame_rate=30)
        else:
            self.ultralytics_tracker = BYTETracker(args=cfg, frame_rate=30)

        # State management
        self.tracks: Dict[int, Dict[str, Any]] = {}
        self.gpu_frame = cv2.cuda_GpuMat()
        self.gpu_gray = cv2.cuda_GpuMat()
        self.prev_gpu_gray = cv2.cuda_GpuMat()
        self.has_prev_gray = False
        
        # Initialize Stream Generator
        self.stream_generator = self.model(source, stream=True, imgsz=self.imgsz, conf=self.conf_th, 
                                           device=self.device, half=(self.device == 0), verbose=False)

        if self.logger:
            self.logger.info(f"YoloTracker initialized for class: {self.target_class} from source {source}")

    def update(self):
        """
        Update the tracker with a new frame from the stream
        Uses KLT for inter-frame tracking and YOLO for periodic detection/correction

        Returns:
            A list of tracked objects: [[id, x1, y1, x2, y2, conf, cls], ...]
        """
        try:
            results = next(self.stream_generator)
        except StopIteration:
            if self.logger:
                self.logger.warning("Stream generator stopped")
            return []
        except Exception as e:
            if self.logger:
                self.logger.error(f"Error getting next frame from stream: {e}")
            return []
            
        # image = stream_results.orig_img
        # boxes = stream_results.boxes

        # Return number of detections
        return len(results)

        if boxes is None:
            h, w = image.shape[:2]
            empty = np.zeros((0, 6), dtype=np.float32)
            boxes = Boxes(empty, orig_shape=(h, w))

        if self.target_cls_id is not None and len(boxes) > 0:
            mask = (boxes.cls.int() == int(self.target_cls_id))
            boxes = boxes[mask]

        h, w = image.shape[:2]
        now = time.monotonic()
        
        # Upload to GPU for KLT
        self.gpu_frame.upload(image)
        cv2.cuda.cvtColor(self.gpu_frame, cv2.COLOR_BGR2GRAY, self.gpu_gray)

        # 1. Try KLT update if we have previous frame and active tracks
        if self.has_prev_gray and self.tracks and self.klt_gpu:
            for tid, tr in list(self.tracks.items()):
                if not tr.get("active", True):
                    continue
                    
                new_pts_gpu, dxy, ok_flow = self.klt_gpu.update(self.prev_gpu_gray, self.gpu_gray, tr["pts_gpu"])
                if ok_flow:
                    dx, dy = dxy
                    x1, y1, x2, y2 = tr["bbox"]
                    tr["bbox"] = self._clamp_xyxy((x1 + dx, y1 + dy, x2 + dx, y2 + dy), w, h)
                    tr["pts_gpu"] = new_pts_gpu
                else:
                    # Flow failed, re-init points
                    tr["pts_gpu"] = self.klt_gpu.init_points(self.gpu_gray, tr["bbox"])

        # 2. Perform ByteTrack/BOT-SORT update using stream detection
        det_np = boxes.cpu().numpy()
        
        # Update the high-level tracker (ByteTrack/BOT-SORT)
        tracked_objects = self.ultralytics_tracker.update(det_np, image, None)

        # Mark all current tracks as inactive before updating with new detections
        for tid in self.tracks.keys():
            self.tracks[tid]["active"] = False

        # 3. Process tracker output
        results = []
        if tracked_objects is not None and len(tracked_objects) > 0:
            for row in tracked_objects:
                x1, y1, x2, y2, track_id, score, cls_id, _idx = row.tolist()
                tid = int(track_id)
                bb = self._clamp_xyxy((x1, y1, x2, y2), w, h)

                if tid not in self.tracks:
                    self.tracks[tid] = {
                        "bbox": bb, 
                        "pts_gpu": None, 
                        "conf": float(score), 
                        "cls": int(cls_id),
                        "active": True, 
                        "t_det": now
                    }
                
                tr = self.tracks[tid]
                tr["bbox"] = bb
                tr["conf"] = float(score)
                tr["cls"] = int(cls_id)
                tr["active"] = True
                tr["t_det"] = now
                
                # Initialize/Update KLT points for this track
                if self.klt_gpu:
                    tr["pts_gpu"] = self.klt_gpu.init_points(self.gpu_gray, bb)
                
                results.append([tid, x1, y1, x2, y2, score, cls_id])

        # Save current gray frame for next KLT update
        self.gpu_gray.copyTo(self.prev_gpu_gray)
        self.has_prev_gray = True
        
        return results

    def _get_target_cls_id(self, names, target_name):
        """Find the class ID for a given class name"""
        if isinstance(names, dict):
            for k, v in names.items():
                if v == target_name:
                    return int(k)
        else:
            for i, v in enumerate(names):
                if v == target_name:
                    return int(i)
        return None
    def _clamp_xyxy(self, bb, w, h):
        """Clamp bounding box to image dimensions."""
        x1, y1, x2, y2 = bb
        x1 = float(max(0, min(w - 1, x1)))
        y1 = float(max(0, min(h - 1, y1)))
        x2 = float(max(0, min(w - 1, x2)))
        y2 = float(max(0, min(h - 1, y2)))
        if x2 <= x1 + 1: x2 = x1 + 2
        if y2 <= y1 + 1: y2 = y1 + 2
        return (x1, y1, x2, y2)