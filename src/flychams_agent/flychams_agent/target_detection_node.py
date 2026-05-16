#!/usr/bin/env python3

"""
Target detection node for the FlyingChameleons system
Author: Jose Francisco Lopez Ruiz
Date: 2026-02-12
"""

import rclpy
from rclpy.node import Node
from rclpy.executors import SingleThreadedExecutor

# Import target detection module
from flychams_agent.detection.target_detection import TargetDetection

class TargetDetectionNode(Node):
    """Target detection node that detects targets based on camera feeds"""
    def __init__(self):
        super().__init__('target_detection_node',
                        allow_undeclared_parameters=True,
                        automatically_declare_parameters_from_overrides=True)

        # Get node parameters
        self.agent_id = self.get_parameter('agent_id').get_parameter_value().string_value
        
        # Create target detection module
        self.module = TargetDetection(self)
        
        self.get_logger().info(f"TargetDetectionNode initialized for {self.agent_id}")

    def on_shutdown(self):
        """Cleanup on shutdown"""
        self.module.shutdown()
        self.get_logger().info("TargetDetectionNode shutting down")

def main(args=None):
    rclpy.init(args=args)
    
    node = TargetDetectionNode()

    executor = SingleThreadedExecutor()
    executor.add_node(node)
    
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        node.on_shutdown()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()