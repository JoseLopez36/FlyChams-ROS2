from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
import os
import yaml

def launch_setup(context, *args, **kwargs):
    # Get is_sim value from LaunchConfiguration
    is_simulated_str = LaunchConfiguration('is_sim').perform(context)
    # Convert string to boolean for use_sim_time parameter
    is_sim = is_simulated_str.lower() in ('true', '1', 'yes', 'on')

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
    operator_path = PathJoinSubstitution([
        'config',
        'package',
        'operator.yaml'
    ])

    # Rviz parameters
    rviz_path = PathJoinSubstitution([
        'config',
        'rviz',
        'default.rviz'
    ])

    # Set environment variable to control ROS logger output
    os.environ['RCUTILS_LOGGING_USE_STDOUT'] = '1' # Enable logging to stdout
    os.environ['RCUTILS_COLORIZED_OUTPUT'] = '1'   # Enable colored output

    # Set environment variables to control Python logging
    # Log level: DEBUG, INFO, WARNING, ERROR, CRITICAL
    os.environ.setdefault('PYTHON_LOG_LEVEL', 'INFO')
    # Enable logging for camera panel module
    os.environ.setdefault('PYTHON_LOG_MODULES', 'flychams_operator.interface.camera_panel')

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

    # Add Operator Interface node
    ld.append(
        Node(
            package='flychams_operator',
            executable='operator_interface_node',
            name='operator_interface_node',
            output='screen',
            namespace='flychams/operator',
            parameters=[
                system_path, 
                topics_path, 
                frames_path, 
                operator_path,
                mission_path
            ]
        )
    )

    # Conditionally add Metrics Factory node
    if is_enabled('metrics_factory'):
        ld.append(
            Node(
                package='flychams_operator',
                executable='metrics_factory_node',
                name='metrics_factory_node',
                output='screen',
                namespace='flychams/operator',
                arguments=['--ros-args', '--log-level', log_level('metrics_factory')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    operator_path,
                    mission_path,
                    {'use_sim_time': is_sim}
                ]
            )
        )

    # Conditionally add Marker Factory node
    if is_enabled('marker_factory'):
        ld.append(
            Node(
                package='flychams_operator',
                executable='marker_factory_node',
                name='marker_factory_node',
                output='screen',
                namespace='flychams/operator',
                arguments=['--ros-args', '--log-level', log_level('marker_factory')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    operator_path,
                    mission_path,
                    {'use_sim_time': is_sim}
                ]
            )
        )

    # Conditionally add Rviz
    if is_enabled('rviz'):
        rviz_config_path = rviz_path.perform(context).strip()
        ld.append(
            Node(
                package='rviz2',
                executable='rviz2',
                name='rviz2',
                output='screen',
                arguments=[
                    '-d', rviz_config_path,
                    '--ros-args',
                    '--log-level', log_level('rviz')
                ],
                parameters=[
                    {'use_sim_time': is_sim}
                ]
            )
        )

    return ld

def generate_launch_description():
    # Declare arguments
    is_simulated_arg = DeclareLaunchArgument(
        'is_sim',
        default_value='True',
        description='Whether the system is simulated'
    )

    return LaunchDescription([
        is_simulated_arg,
        OpaqueFunction(function=launch_setup)
    ])
