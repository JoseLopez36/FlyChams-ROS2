"""
Target detection module for the FlyingChameleons system
Author: Jose Francisco Lopez Ruiz
Date: 2026-03-02
"""

import rclpy
import threading
from rclpy.node import Node

# Import tracker modules
from flychams_agent.detection.yolo_tracker import YoloTracker
from flychams_agent.detection.vision_utils import VisionUtils

class TargetDetection:
    """Module responsible for ROS2 lifecycle and orchestration of target detection"""
    def __init__(self, node: Node):
        self.node = node
        self.logger = node.get_logger()
        self.vision_utils = VisionUtils()
        
        # --- Parameters ---
        self.source = self.node.get_parameter('source').get_parameter_value().string_value
        self.detector_model = self.node.get_parameter('detector_model').get_parameter_value().string_value
        self.target_class = self.node.get_parameter('target_class').get_parameter_value().string_value
        
        # --- State ---
        self.is_running = True
        
        # --- Tracker Initialization ---
        self.tracker = YoloTracker(
            model_name=self.detector_model,
            target_class=self.target_class, 
            source=self.source, 
            logger=self.logger
        )
        
        # --- Processing Thread ---
        self.process_thread = threading.Thread(target=self._process_loop)
        self.process_thread.daemon = True
        self.process_thread.start()
        
        self.logger.info(f"TargetDetection module initialized")

    def _process_loop(self):
        """Continuously poll tracker stream and publish positions"""
        while self.is_running and rclpy.ok():
            try:
                # Update tracker
                tracked_objects = self.tracker.update()
                self.logger.info(f"Tracked objects: {tracked_objects}")
            except Exception as e:
                self.logger.error(f"Tracker update failed: {e}")
                continue
                
            if tracked_objects is None:
                continue
                
            # Calculate 3D positions and prepare messages
            # pose_array_msg = PoseArray()
            # pose_array_msg.header.stamp = self.node.get_clock().now().to_msg()
            # pose_array_msg.header.frame_id = "map"  # Or your appropriate global frame
            
            # for obj in tracked_objects:
            #     tid, x1, y1, x2, y2, score, cls_id = obj
                
            #     # Calculate center (u, v) - using bottom center for ground projection
            #     u = 0.5 * (x1 + x2)
            #     v = y2
                
            #     # Project to 3D
            #     calculated_pos = self.vision_utils.calculate_3d_position(
            #         u, v, self.K, pose, self.z_plane
            #     )
                
            #     if calculated_pos is not None:
            #         tx, ty = calculated_pos
                    
            #         # Create Pose for PoseArray
            #         p = Pose()
            #         p.position.x = tx
            #         p.position.y = ty
            #         p.position.z = self.z_plane
            #         pose_array_msg.poses.append(p)

            # Publish results
            # self.pose_array_pub.publish(pose_array_msg)

    def shutdown(self) -> None:
        """Cleanup resources"""
        self.is_running = False
        if self.process_thread.is_alive():
            self.process_thread.join(timeout=1.0)
        self.logger.info("TargetDetection module shutting down")