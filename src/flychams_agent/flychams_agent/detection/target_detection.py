"""
Target detection module for the FlyingChameleons system
Author: Jose Francisco Lopez Ruiz
Date: 2026-02-12
"""

import rclpy
from rclpy.node import Node

# Import tracker modules
from .tracker import Tracker
from .yolo_tracker import YoloTracker

class TargetDetection:
    """Module responsible for ROS2 lifecycle and orchestration of target detection"""
    def __init__(self, node: Node):
        self.node = node
        self.logger = node.get_logger()
        
        # --- Parameters ---
        self.node.declare_parameter('detector_type', 'yolo')
        self.node.declare_parameter('detector_model', 'yolov11n.pt')
        self.detector_type = self.node.get_parameter('detector_type').get_parameter_value().string_value
        self.detector_model = self.node.get_parameter('detector_model').get_parameter_value().string_value
        
        # --- Tracker Initialization ---
        self.tracker = self._create_tracker(self.detector_type)

        # --- Subscribers ---
        
        # --- Publishers ---
        
        self.logger.info(f"TargetDetection module initialized with {self.detector_type} tracker")

    def _create_tracker(self, detector_type: str):
        """Factory method to create the requested tracker"""
        if detector_type.lower() == 'yolo':
            return YoloTracker()
        else:
            self.logger.error(f"Unknown detector type: {detector_type}")
            return None

    def shutdown(self) -> None:
        """Cleanup resources"""
        self.logger.info("TargetDetection module shutting down")