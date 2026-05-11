from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import OpaqueFunction
from launch.substitutions import PathJoinSubstitution
import os
import yaml

def launch_setup(context, *args, **kwargs):
    # Get paths to config files
    # Core parameters
    system_path = PathJoinSubstitution([
        'config',
        'core',
        'system.yaml'
    ])
    launch_path = PathJoinSubstitution([
        'config',
        'core',
        'launch.yaml'
    ])
    topics_path = PathJoinSubstitution([
        'config',
        'core',
        'topics.yaml'
    ])
    frames_path = PathJoinSubstitution([
        'config',
        'core',
        'frames.yaml'
    ])
    
    # Mission parameters
    mission_path = PathJoinSubstitution([
        'config',
        'generated',
        'mission.yaml'
    ])
    
    # Package parameters
    simulation_path = PathJoinSubstitution([
        'config',
        'package',
        'simulation.yaml'
    ])

    # Set environment variable to control ROS logger output
    os.environ['RCUTILS_LOGGING_USE_STDOUT'] = '1' # Enable logging to stdout
    os.environ['RCUTILS_COLORIZED_OUTPUT'] = '1'   # Enable colored output

    # Generate launch description
    ld = []

    # Load the nodes configuration YAML file
    launch_file_path = launch_path.perform(context).strip()
    with open(launch_file_path, 'r') as f:
        launch_raw = yaml.safe_load(f) or {}

    # Get parameters from the launch file
    node_allowlist = launch_raw.get('node_allowlist', [])
    node_log_levels = launch_raw.get('node_log_levels', {})

    def is_enabled(node_name: str) -> bool:
        return node_name in node_allowlist

    def log_level(node_name: str) -> str:
        return node_log_levels.get(node_name, 'info')

    # Add Dataset Generator node
    ld.append(
        Node(
            package='flychams_simulation',
            executable='dataset_generator_node',
            name='dataset_generator_node',
            output='screen',
            namespace='flychams/simulation',
            parameters=[
                system_path, 
                topics_path, 
                frames_path, 
                simulation_path,
                mission_path,
                {'use_sim_time': True}
            ]
        )
    )

    return ld

def generate_launch_description():
    return LaunchDescription([
        OpaqueFunction(function=launch_setup)
    ])
