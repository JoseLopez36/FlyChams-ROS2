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
    coordinator_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'coordinator.yaml'
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

    # Conditionally add Element Registrator node
    if is_enabled('element_registrator'):
        ld.append(
            Node(
                package='flychams_coordinator',
                executable='element_registrator_node',
                name='element_registrator_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('element_registrator')],
                parameters=[
                    system_path, 
                    topics_path,
                    frames_path,
                    coordinator_path,
                    mission_params_path
                ]
            )
        )
    
    # Conditionally add Target Clustering node
    if is_enabled('target_clustering'):
        ld.append(
            Node(
                package='flychams_coordinator',
                executable='target_clustering_node',
                name='target_clustering_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('target_clustering')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordinator_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Cluster Analysis node
    if is_enabled('cluster_analysis'):
        ld.append(
            Node(
                package='flychams_coordinator',
                executable='cluster_analysis_node',
                name='cluster_analysis_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('cluster_analysis')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordinator_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Agent Assignment node
    if is_enabled('agent_assignment'):
        ld.append(
            Node(
                package='flychams_coordinator',
                executable='agent_assignment_node',
                name='agent_assignment_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('agent_assignment')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordinator_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
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
