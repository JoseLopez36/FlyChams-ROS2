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

    # Conditionally add AirSim node
    if is_enabled('airsim'):
        ld.append(
            Node(
                package='airsim_wrapper',
                executable='airsim_node',
                name='airsim_node',
                output='screen',
                namespace='airsim',
                arguments=['--ros-args', '--log-level', log_level('airsim')],
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
        )

    # Conditionally add Simulation GUI node
    if is_enabled('simulation_gui'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='simulation_gui_node',
                name='simulation_gui_node',
                output='screen',
                namespace='flychams/simulation',
                arguments=['--ros-args', '--log-level', log_level('simulation_gui')],
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

    # Conditionally add Target State node
    if is_enabled('target_state'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='target_state_node',
                name='target_state_node',
                output='screen',
                namespace='flychams/simulation',
                arguments=['--ros-args', '--log-level', log_level('target_state')],
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

    # Conditionally add Target Control node
    if is_enabled('target_control'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='target_control_node',
                name='target_control_node',
                output='screen',
                namespace='flychams/simulation',
                arguments=['--ros-args', '--log-level', log_level('target_control')],
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
