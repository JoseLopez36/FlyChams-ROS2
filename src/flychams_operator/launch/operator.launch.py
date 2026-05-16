#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
import os
import yaml

def launch_foxglove_bridge():
    port = int(os.environ.get('FOXGLOVE_PORT', 8765))
    return [
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
                    '^/flychams/(?!coordinator/(start_mission|pause_mission|abort_mission|arm_all|return_home)).*',
                    '/rosout',
                ],
                'client_publish_topic_whitelist': [
                    '/flychams/coordinator/start_mission',
                    '/flychams/coordinator/pause_mission',
                    '/flychams/coordinator/abort_mission',
                    '/flychams/coordinator/arm_all',
                    '/flychams/coordinator/return_home',
                ],
                'send_buffer_limit': 10000000,
                'num_threads': 4,
            }]
        )
    ]

def generate_launch_description():
    # Resolve config directories
    common_core_dir = os.path.join(
        get_package_share_directory('flychams_common'), 'config', 'core')
    common_generated_dir = os.path.join(
        get_package_share_directory('flychams_common'), 'config', 'generated')
    pkg_config_dir = os.path.join(
        get_package_share_directory('flychams_operator'), 'config')

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

    # Define node configurations
    node_configs = {
        'metrics_creator': {
            'executable': 'metrics_creator_node',
            'name': 'metrics_creator_node'
        },
        'markers_generator': {
            'executable': 'markers_generator_node',
            'name': 'markers_generator_node'
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
                package='flychams_operator',
                executable=config['executable'],
                name=config['name'],
                namespace='flychams/operator',
                parameters=common_params,
                output='screen',
                arguments=['--ros-args', '--log-level', log_level]
            )
            nodes.append(node)

    # Launch Foxglove Bridge
    foxglove_nodes = launch_foxglove_bridge()

    return LaunchDescription([
        mission_yaml,
        *nodes,
        *foxglove_nodes,
    ])