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

    # Read clock_speed from system.yaml to determine if sim time is needed
    system_yaml_path = os.path.join(common_core_dir, 'system.yaml')
    with open(system_yaml_path, 'r') as f:
        system_config = yaml.safe_load(f)
    clock_speed = system_config.get('/**', {}).get('ros__parameters', {}).get('simulation', {}).get('clock_speed', 1.0)
    use_sim_time = clock_speed != 1.0

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
        {'use_sim_time': use_sim_time},
    ]

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
        'agent_bridge': {
            'executable': 'agent_bridge_node',
            'name': 'agent_bridge_node'
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
        *nodes
    ])