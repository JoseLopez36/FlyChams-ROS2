"""
Target detection module for the FlyingChameleons system
Author: Jose Francisco Lopez Ruiz
Date: 2026-03-02
"""

import rclpy
import threading
from cv_bridge import CvBridge
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSReliabilityPolicy
from sensor_msgs.msg import Image
from std_msgs.msg import Float64MultiArray, Float32MultiArray
from geometry_msgs.msg import PoseArray, Pose

# Import tracker modules
from .yolo_tracker import YOLOTracker
from .vision_utils import VisionUtils

class TargetDetection:
    """Module responsible for ROS2 lifecycle and orchestration of target detection"""
    def __init__(self, node: Node):
        self.node = node
        self.logger = node.get_logger()
        self.bridge = CvBridge()
        self.vision_utils = VisionUtils()
        
        # --- Parameters ---
        self.node.declare_parameter('detector_model', 'yolo11m.engine')
        self.node.declare_parameter('target_class', 'person')
        self.node.declare_parameter('image_topic', '/camera/image_raw')
        self.node.declare_parameter('pose_topic', '/vision/transform')
        self.node.declare_parameter('tracks_topic', '/vision/tracks_xy')
        self.node.declare_parameter('fov_h_deg', 81.0)
        self.node.declare_parameter('z_plane', 0.0)
        
        self.detector_model = self.node.get_parameter('detector_model').get_parameter_value().string_value
        self.target_class = self.node.get_parameter('target_class').get_parameter_value().string_value
        self.image_topic = self.node.get_parameter('image_topic').get_parameter_value().string_value
        self.pose_topic = self.node.get_parameter('pose_topic').get_parameter_value().string_value
        self.tracks_topic = self.node.get_parameter('tracks_topic').get_parameter_value().string_value
        self.fov_h_deg = self.node.get_parameter('fov_h_deg').get_parameter_value().double_value
        self.z_plane = self.node.get_parameter('z_plane').get_parameter_value().double_value
        
        # --- State ---
        self.lock = threading.Lock()
        self.current_pose = None # [x, y, z, yaw, pitch, roll]
        self.K = None # Camera intrinsics
        
        # --- Tracker Initialization ---
        self.tracker = YOLOTracker(model_path=self.detector_model, target_class=self.target_class, logger=self.logger)

        # --- QoS Profile ---
        qos = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            reliability=QoSReliabilityPolicy.BEST_EFFORT
        )

        # --- Subscribers ---
        self.img_sub = self.node.create_subscription(
            Image, 
            self.image_topic, 
            self._image_callback, 
            qos
        )
        self.pose_sub = self.node.create_subscription(
            Float64MultiArray, 
            self.pose_topic, 
            self._pose_callback, 
            qos
        )
        
        # --- Publishers ---
        self.pose_array_pub = self.node.create_publisher(
            PoseArray, 
            self.tracks_topic, 
            qos
        )
        
        self.logger.info(f"TargetDetection module initialized")

    def _pose_callback(self, msg: Float64MultiArray):
        """Update the current drone pose"""
        if msg.data is None:
            return
        with self.lock:
            self.current_pose = list(msg.data[:6])

    def _image_callback(self, msg: Image):
        """Process incoming images, track targets, and calculate 3D positions"""
        if self.tracker is None:
            return

        # 1. Convert ROS Image to CV2
        try:
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        except Exception as e:
            self.logger.error(f"Failed to convert image: {e}")
            return

        # 2. Initialize Intrinsics if not done
        if self.K is None:
            h, w = cv_image.shape[:2]
            self.K = self.vision_utils.build_k_from_hfov(w, h, self.fov_h_deg)

        # 3. Get current pose
        with self.lock:
            pose = self.current_pose
        
        if pose is None:
            self.logger.warn("No pose data available yet, skipping detection")
            return

        # 4. Update Tracker
        try:
            tracked_objects = self.tracker.update(cv_image)
        except Exception as e:
            self.logger.error(f"Tracker update failed: {e}")
            return

        # 5. Calculate 3D positions and prepare messages
        pose_array_msg = PoseArray()
        pose_array_msg.header = msg.header
        
        for obj in tracked_objects:
            tid, x1, y1, x2, y2, score, cls_id = obj
            
            # Calculate center (u, v) - using bottom center for ground projection
            u = 0.5 * (x1 + x2)
            v = y2
            
            # Project to 3D
            calculated_pos = self.vision_utils.calculate_3d_position(
                u, v, self.K, pose, self.z_plane
            )
            
            if calculated_pos is not None:
                tx, ty = calculated_pos
                
                # Create Pose for PoseArray
                p = Pose()
                p.position.x = tx
                p.position.y = ty
                p.position.z = self.z_plane
                pose_array_msg.poses.append(p)

        # 6. Publish results
        self.pose_array_pub.publish(pose_array_msg)

    def shutdown(self) -> None:
        """Cleanup resources"""
        self.logger.info("TargetDetection module shutting down")