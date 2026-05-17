#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node
import os

def generate_launch_description():
    port = int(os.environ.get('FOXGLOVE_PORT', 8765))
    
    return LaunchDescription([
        Node(
            package='foxglove_bridge',
            executable='foxglove_bridge',
            name='foxglove_bridge',
            output='screen',
            namespace='foxglove',
            arguments=['--ros-args', '--log-level', 'info'],
            parameters=[{
                'port': port,
                'address': '0.0.0.0',
                'tls': False,
                'topic_whitelist': [
                    '^/flychams/(?!coordinator/(start_mission|pause_mission|abort_mission|arm_all|return_home|land_all)).*',
                    '/rosout',
                ],
                'client_publish_topic_whitelist': [
                    '/flychams/coordinator/start_mission',
                    '/flychams/coordinator/pause_mission',
                    '/flychams/coordinator/abort_mission',
                    '/flychams/coordinator/arm_all',
                    '/flychams/coordinator/return_home',
                    '/flychams/coordinator/land_all',
                ],
                'send_buffer_limit': 10000000,
                'max_update_ms': 100
            }]
        )
    ])