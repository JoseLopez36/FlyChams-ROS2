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
            arguments=['--ros-args', '--log-level', 'warn'],
            parameters=[{
                'port': port,
                'address': '0.0.0.0',
                'tls': False,
                # Threading
                'num_threads': 16,
                # QoS
                'min_qos_depth': 1,
                'max_qos_depth': 30000,
                # Restrict advertised topics and services to flychams namespace
                'topic_whitelist': ['^/flychams/.*', '/clock', '/rosout', '/tf', '/tf_static'],
                'service_whitelist': ['^/flychams/.*'],
                # Allow coordinator command publishers from Foxglove
                'client_topic_whitelist': [
                    '/flychams/coordinator/start_mission',
                    '/flychams/coordinator/pause_mission',
                    '/flychams/coordinator/abort_mission',
                    '/flychams/coordinator/arm_all',
                    '/flychams/coordinator/return_home',
                    '/flychams/coordinator/land_all',
                ],
                # Send buffer: 100 MB
                'send_buffer_limit': 100000000,
            }]
        )
    ])