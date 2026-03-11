"""
Target detection module for the FlyingChameleons system
Author: Jose Francisco Lopez Ruiz
Date: 2026-03-02
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseArray, Pose

# Import tracker modules
from flychams_agent.detection.yolo_tracker import YoloTracker
from flychams_agent.detection.vision_utils import VisionUtils

class TargetDetection:
    def __init__(self, node: Node):
        self.node = node
        self.logger = node.get_logger()
        self.vision_utils = VisionUtils()
        
        # --- Parameters ---
        self.inference_rate = self.node.get_parameter('inference_rate').get_parameter_value().double_value
        self.detector_model = self.node.get_parameter('detector_model').get_parameter_value().string_value
        self.target_class = self.node.get_parameter('target_class').get_parameter_value().string_value
        self.width = self.node.get_parameter('width').get_parameter_value().int_value
        self.height = self.node.get_parameter('height').get_parameter_value().int_value
        self.z_plane = self.node.get_parameter('z_plane').get_parameter_value().double_value

        # Get agent ID from parameters
        self.agent_id = self.node.get_parameter('agent_id').get_parameter_value().string_value

        # Get source URL from parameters
        self.source = self.node.get_parameter('agents.' + self.agent_id + '.inference_stream_url').get_parameter_value().string_value
        
        # Placeholder for camera intrinsics and drone pose (to be updated via subscribers/TF)
        self.K = self.vision_utils.build_k_from_hfov(self.width, self.height, 90.0)
        self.camera_pose = [0.0, 0.0, 10.0, 0.0, 0.0, 0.0] # x, y, z, yaw, pitch, roll
        
        # --- Publishers ---
        # Using a generic topic name for now, should ideally come from topics.yaml
        self.pose_array_pub = self.node.create_publisher(PoseArray, 'detected_targets', 10)
        
        # --- Tracker Initialization ---
        self.tracker = YoloTracker(
            model_name=self.detector_model,
            target_class=self.target_class, 
            source=self.source, 
            logger=self.logger
        )
        
        # --- Processing Timer ---
        timer_period = 1.0 / self.inference_rate
        self.timer = self.node.create_timer(timer_period, self.update)
        
        self.logger.info(f"TargetDetection module initialized")

    def update(self):
        # Update tracker
        try:
            tracked_objects = self.tracker.update()
            self.logger.info(f"Number of tracked objects: {len(tracked_objects)}")
        except Exception as e:
            self.logger.error(f"Tracker update failed: {e}")
            return

        if not tracked_objects or len(tracked_objects) == 0:
            return
            
        # Prepare messages
        pose_array_msg = PoseArray()
        pose_array_msg.header.stamp = self.node.get_clock().now().to_msg()
        pose_array_msg.header.frame_id = "world"
        
        # Calculate 3D positions
        for obj in tracked_objects:
            tid, x1, y1, x2, y2, score, cls_id = obj
            
            # Calculate center (u, v) - using bottom center for ground projection
            u = 0.5 * (x1 + x2)
            v = y2
            
            # Project to 3D
            calculated_pos = self.vision_utils.calculate_3d_position(
                u, v, self.K, self.drone_pose, self.z_plane
            )
            
            if calculated_pos is not None:
                tx, ty = calculated_pos
                
                # Create Pose for PoseArray
                p = Pose()
                p.position.x = tx
                p.position.y = ty
                p.position.z = self.z_plane
                pose_array_msg.poses.append(p)

        # Publish results
        self.pose_array_pub.publish(pose_array_msg)

    def shutdown(self) -> None:
        self.logger.info("TargetDetection module shutting down")