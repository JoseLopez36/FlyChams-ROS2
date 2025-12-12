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
    # Get is_simulated value from LaunchConfiguration
    is_simulated_str = LaunchConfiguration('is_simulated').perform(context)
    # Convert string to boolean for use_sim_time parameter
    is_simulated = is_simulated_str.lower() in ('true', '1', 'yes', 'on')
    
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
    bringup_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'bringup.yaml'
    ])
    control_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'control.yaml'
    ])
    coordination_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'coordination.yaml'
    ])
    perception_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'perception.yaml'
    ])
    simulation_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'simulation.yaml'
    ])

    # Set environment variable to control ROS logger output
    os.environ['RCUTILS_LOGGING_USE_STDOUT'] = '0' # Disable logging to stdout
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
    
    # ============= BRINGUP NODES =============
    # Conditionally add Element Registrator node
    if is_enabled('element_registrator'):
        ld.append(
            Node(
                package='flychams_bringup',
                executable='element_registrator_node',
                name='element_registrator_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('element_registrator')],
                parameters=[
                    system_path, 
                    topics_path,
                    frames_path,
                    bringup_path,
                    mission_params_path
                ]
            )
        )

    # ============= AIRSIM NODES =============
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

    return ld

def generate_launch_description():
    # Declare arguments
    is_simulated_arg = DeclareLaunchArgument(
        'is_simulated',
        default_value='True',
        description='Whether the system is simulated'
    )

    return LaunchDescription([
        is_simulated_arg,
        OpaqueFunction(function=launch_setup)
    ])
