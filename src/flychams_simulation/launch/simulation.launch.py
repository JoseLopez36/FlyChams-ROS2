#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
import os
import yaml

def generate_launch_description():
    # Declare launch arguments
    config_dir = DeclareLaunchArgument(
        'config_dir',
        default_value=os.path.join(
            get_package_share_directory('flychams_simulation'), 'config'),
        description='Path to simulation configuration directory'
    )

    # Load launch configuration
    launch_config_path = os.path.join(
        get_package_share_directory('flychams_simulation'), 'config', 'launch.yaml')
    
    with open(launch_config_path, 'r') as f:
        launch_config = yaml.safe_load(f)

    node_allowlist = launch_config.get('node_allowlist', [])
    node_log_levels = launch_config.get('node_log_levels', {})

    # Define node configurations
    node_configs = {
        'airsim': {
            'executable': 'airsim_node',
            'name': 'airsim_node'
        },
        'simulation_gui': {
            'executable': 'simulation_gui_node',
            'name': 'simulation_gui_node'
        },
        'target_state': {
            'executable': 'target_state_node',
            'name': 'target_state_node'
        },
        'target_control': {
            'executable': 'target_control_node',
            'name': 'target_control_node'
        }
    }

    # Create nodes based on allowlist
    nodes = []
    for node_name in node_allowlist:
        if node_name in node_configs:
            config = node_configs[node_name]
            
            # Set log level if specified
            log_level = node_log_levels.get(node_name, 'info')
            
            node = Node(
                package='flychams_simulation',
                executable=config['executable'],
                name=config['name'],
                parameters=[
                    LaunchConfiguration('config_dir'),
                    {'log_level': log_level}
                ],
                output='screen',
                arguments=['--ros-args', '--log-level', log_level]
            )
            nodes.append(node)

    return LaunchDescription([
        config_dir,
        *nodes
    ])