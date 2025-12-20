from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare
import os
import yaml

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
    
    # Mission parameters
    mission_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'mission.yaml'
    ])
    
    # Package parameters
    dashboard_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'dashboard.yaml'
    ])

    # Rviz parameters
    rviz_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'rviz',
        'default.rviz'
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

    # Conditionally add Metrics Factory node
    if is_enabled('metrics_factory'):
        ld.append(
            Node(
                package='flychams_dashboard',
                executable='metrics_factory_node',
                name='metrics_factory_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('metrics_factory')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    dashboard_path,
                    mission_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Marker Factory node
    if is_enabled('marker_factory'):
        ld.append(
            Node(
                package='flychams_dashboard',
                executable='marker_factory_node',
                name='marker_factory_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('marker_factory')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    dashboard_path,
                    mission_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Rviz
    if is_enabled('rviz'):
        ld.append(
            Node(
                package='rviz2',
                executable='rviz2',
                name='rviz2',
                output='screen',
                arguments=['--ros-args', '--log-level', log_level('rviz')],
                parameters=[
                    rviz_path,
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
