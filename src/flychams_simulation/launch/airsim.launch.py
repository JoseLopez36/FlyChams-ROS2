#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='airsim_wrapper',
            executable='airsim_node',
            name='airsim_node',
            output='screen',
            namespace='airsim',
            arguments=['--ros-args', '--log-level', 'info'],
            parameters=[{
                'update_sim_clock_every_n_sec': 0.01,
                'host_ip': 'localhost',
                'host_port': 41451
            }]
        )
    ])