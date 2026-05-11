#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # Resolve core config directory
    common_core_dir = os.path.join(
        get_package_share_directory('flychams_common'), 'config', 'core')

    # Settings creator node
    settings_creator_node = Node(
        package='flychams_common',
        executable='settings_creator_node',
        name='settings_creator_node',
        parameters=[
            os.path.join(common_core_dir, 'system.yaml'),
            os.path.join(common_core_dir, 'topics.yaml'),
            os.path.join(common_core_dir, 'frames.yaml')
        ],
        output='screen'
    )

    return LaunchDescription([
        settings_creator_node
    ])