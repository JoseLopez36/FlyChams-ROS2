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

    # Set environment variable to control ROS logger output
    os.environ['RCUTILS_LOGGING_USE_STDOUT'] = '0' # Disable logging to stdout
    os.environ['RCUTILS_COLORIZED_OUTPUT'] = '1'   # Enable colored output

    # Generate launch description
    ld = []

    # Load the nodes configuration YAML file
    # Convert from PathJoinSubstitution to path string and load the file
    launch_file_path = launch_path.perform(context).strip()
    with open(launch_file_path, 'r') as f:
        launch = yaml.safe_load(f)

    # Get the node activation settings from config
    nodes = {
        # Global setup nodes
        'registrator': launch.get('registrator', [True, 'info']),
        'airsim': launch.get('airsim', [True, 'info'])
    }

    # Conditionally add Registrator node
    if nodes['registrator'][0]:
        ld.append(
            Node(
                package='flychams_bringup',
                executable='registrator_node',
                name='registrator_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', nodes['registrator'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path
                ]
            )
        )

    # Conditionally add AirSim node
    if nodes['airsim'][0]:
        ld.append(
            Node(
                package='airsim_wrapper',
                executable='airsim_node',
                name='airsim_node',
                output='screen',
                namespace='airsim',
                arguments=['--ros-args', '--log-level', nodes['airsim'][1]],
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
