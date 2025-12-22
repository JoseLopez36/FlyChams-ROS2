"""Utility functions for FlyChams Dashboard"""

import rclpy
from rclpy.executors import Executor
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from rclpy.node import Node


def replace_id_in_topic(topic_pattern: str, placeholder: str, element_id: str) -> str:
    """Replace placeholder in topic pattern with element ID"""
    return topic_pattern.replace(placeholder, element_id)


def spin_ros_node(node: 'Node', executor: Executor) -> None:
    """Run ROS2 executor in a separate thread"""
    try:
        executor.add_node(node)
        executor.spin()
    except Exception as e:
        node.get_logger().error(f'Error in ROS2 executor thread: {e}')
    finally:
        executor.shutdown()

