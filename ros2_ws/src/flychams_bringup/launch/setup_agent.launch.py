from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
import os
    
def generate_launch_description():
    # Get paths to config files
    # Core parameters
    system_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'core',
        'system.yaml'
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
    mavros_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'mavros',
        'mavros.yaml'
    ])
    plugin_lists_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'mavros',
        'pluginlists.yaml'
    ])

    # Set environment variable to control ROS logger output
    os.environ['RCUTILS_LOGGING_USE_STDOUT'] = '0' # Disable logging to stdout
    os.environ['RCUTILS_COLORIZED_OUTPUT'] = '1'   # Enable colored output

    # Generate launch description
    ld = []

    # Add mavROS Manager node
    ld.append(
        Node(
            package='flychams_bringup',
            executable='mavros_manager_node',
            name='mavros_manager_node',
            output='screen',
            namespace='flychams',
            parameters=[
                system_path,
                topics_path,
                frames_path,
                mavros_path,
                plugin_lists_path,
                {'tgt_system': 1, 'fcu_url': 'udp://:14030@172.17.0.2:14280'}
            ]
        )
    )

    return LaunchDescription(ld)