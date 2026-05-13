#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
import os
import yaml

def generate_launch_description():
    # Resolve config directories
    common_core_dir = os.path.join(
        get_package_share_directory('flychams_common'), 'config', 'core')
    common_generated_dir = os.path.join(
        get_package_share_directory('flychams_common'), 'config', 'generated')
    pkg_config_dir = os.path.join(
        get_package_share_directory('flychams_simulation'), 'config')

    # Declare launch arguments
    mission_yaml = DeclareLaunchArgument(
        'mission_yaml',
        default_value=os.path.join(common_generated_dir, 'mission.yaml'),
        description='Path to generated mission YAML'
    )

    # Load launch configuration
    launch_config_path = os.path.join(pkg_config_dir, 'launch.yaml')
    with open(launch_config_path, 'r') as f:
        launch_config = yaml.safe_load(f)

    node_allowlist = launch_config.get('node_allowlist', [])
    node_log_levels = launch_config.get('node_log_levels', {})

    # Common parameter files loaded by every node (in merge order)
    common_params = [
        os.path.join(common_core_dir, 'system.yaml'),
        os.path.join(common_core_dir, 'topics.yaml'),
        os.path.join(common_core_dir, 'frames.yaml'),
        LaunchConfiguration('mission_yaml'),
        os.path.join(pkg_config_dir, 'nodes.yaml'),
    ]

    # AirSim node
    airsim_node = Node(
        package='airsim_wrapper',
        executable='airsim_node',
        name='airsim_node',
        output='screen',
        namespace='airsim',
        arguments=['--ros-args', '--log-level', 'error'],
        parameters=[{
            'update_airsim_state_every_n_sec': 0.020,
            'update_sim_clock_every_n_sec': 0.001,
            'world_frame_id': 'world',
            'vehicle_local_frame_id': 'local',
            'vehicle_body_frame_id': 'body',
            'camera_body_frame_id': 'body',
            'camera_optical_frame_id': 'optical',
            'host_ip': 'localhost',
            'host_port': 41451,
            'broadcast_transforms': False
        }]
    )

    # Define node configurations
    node_configs = {
        'target_state': {
            'executable': 'target_state_node',
            'name': 'target_state_node'
        },
        'target_control': {
            'executable': 'target_control_node',
            'name': 'target_control_node'
        },
        'camera_manager': {
            'executable': 'camera_manager_node',
            'name': 'camera_manager_node'
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
                namespace='flychams/simulation',
                parameters=[
                    *common_params,
                    {'log_level': log_level}
                ],
                output='screen',
                arguments=['--ros-args', '--log-level', log_level]
            )
            nodes.append(node)

    return LaunchDescription([
        mission_yaml,
        airsim_node,
        *nodes
    ])