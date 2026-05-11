#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, EnvironmentVariable
from ament_index_python.packages import get_package_share_directory
import os
import yaml

def generate_launch_description():
    # Declare launch arguments
    config_dir = DeclareLaunchArgument(
        'config_dir',
        default_value=os.path.join(
            get_package_share_directory('flychams_agent'), 'config'),
        description='Path to agent configuration directory'
    )

    agent_id = DeclareLaunchArgument(
        'agent_id',
        default_value=EnvironmentVariable('AGENT_ID'),
        description='Agent ID (defaults to AGENT_ID environment variable)'
    )

    # Load launch configuration
    launch_config_path = os.path.join(
        get_package_share_directory('flychams_agent'), 'config', 'launch.yaml')
    
    with open(launch_config_path, 'r') as f:
        launch_config = yaml.safe_load(f)

    node_allowlist = launch_config.get('node_allowlist', [])
    node_log_levels = launch_config.get('node_log_levels', {})

    # Define node configurations
    node_configs = {
        'mavros_manager': {
            'executable': 'mavros_manager_node',
            'name': 'mavros_manager_node'
        },
        'drone_control': {
            'executable': 'drone_control_node',
            'name': 'drone_control_node'
        },
        'drone_state': {
            'executable': 'drone_state_node',
            'name': 'drone_state_node'
        },
        'drone_frames': {
            'executable': 'drone_frames_node',
            'name': 'drone_frames_node'
        },
        'camera_control': {
            'executable': 'camera_control_node',
            'name': 'camera_control_node'
        },
        'camera_frames': {
            'executable': 'camera_frames_node',
            'name': 'camera_frames_node'
        },
        'agent_positioning': {
            'executable': 'agent_positioning_node',
            'name': 'agent_positioning_node'
        },
        'agent_tracking': {
            'executable': 'agent_tracking_node',
            'name': 'agent_tracking_node'
        },
        'agent_analysis': {
            'executable': 'agent_analysis_node',
            'name': 'agent_analysis_node'
        },
        'agent_stream': {
            'executable': 'agent_stream_node',
            'name': 'agent_stream_node'
        },
        'target_detection': {
            'executable': 'target_detection_node',
            'name': 'target_detection_node'
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
                package='flychams_agent',
                executable=config['executable'],
                name=config['name'],
                parameters=[
                    LaunchConfiguration('config_dir'),
                    {'agent_id': LaunchConfiguration('agent_id')},
                    {'log_level': log_level}
                ],
                output='screen',
                arguments=['--ros-args', '--log-level', log_level]
            )
            nodes.append(node)

    return LaunchDescription([
        config_dir,
        agent_id,
        *nodes
    ])