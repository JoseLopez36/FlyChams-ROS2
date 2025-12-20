from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory
import os
import yaml

def load_mission_parameters(mission_settings_path):
    if not os.path.exists(mission_settings_path):
        return {}
    
    return mission_settings_path

def launch_setup(context, *args, **kwargs):
    # Get paths to config files
    # Core parameters
    system_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'core',
        'system.yaml'
    ])
    launch_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'core',
        'launch.yaml'
    ])
    topics_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'core',
        'topics.yaml'
    ])
    frames_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'core',
        'frames.yaml'
    ])
    
    # Package parameters
    simulation_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
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

    # Load mission parameters
    system_file_path = system_path.perform(context).strip()
    with open(system_file_path, 'r') as f:
        system_raw = yaml.safe_load(f) or {}

    mission_params_path = load_mission_parameters(system_raw.get('/**').get('ros__parameters').get('path').get('mission_settings_path'))

    # Verify mission parameters file exists
    if not mission_params_path or not os.path.exists(mission_params_path):
        raise FileNotFoundError(f"Mission parameters file not found: {mission_params_path}")
    
    print(f"Loading mission parameters from: {mission_params_path}")

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
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('simulation_gui')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
                    mission_params_path,
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
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('target_state')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
                    mission_params_path,
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
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('target_control')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
                    mission_params_path,
                    {'use_sim_time': True}
                ]
            )
        )

    return ld

def generate_launch_description():
    return LaunchDescription([
        OpaqueFunction(function=launch_setup)
    ])
