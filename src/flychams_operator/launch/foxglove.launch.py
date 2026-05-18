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
                # Threading: one thread per image stream (3 agents x 3 cameras = 9), plus margin
                'num_threads': 12,
                # QoS
                'min_qos_depth': 1,
                'max_qos_depth': 2,
                # Force image topics to best_effort to avoid retransmit overhead
                'best_effort_qos_topic_whitelist': [
                    '^/flychams/agent/.*/.*/(image|image/compressed)$',
                ],
                # Restrict advertised topics to /flychams namespace
                'topic_whitelist': ['^/flychams/.*', '/rosout'],
                # Allow coordinator command publishers from Foxglove
                'client_topic_whitelist': [
                    '/flychams/coordinator/start_mission',
                    '/flychams/coordinator/pause_mission',
                    '/flychams/coordinator/abort_mission',
                    '/flychams/coordinator/arm_all',
                    '/flychams/coordinator/return_home',
                    '/flychams/coordinator/land_all',
                ],
                # Send buffer: 100 MB is sufficient for 9 compressed streams
                'send_buffer_limit': 100000000,
                # Push frames as fast as they arrive
                'max_update_ms': 0,
                # Disable unused diagnostics
                'sysinfo': False,
                'use_sim_time': False,
            }]
        )
    ])