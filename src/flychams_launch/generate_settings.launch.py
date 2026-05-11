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
        'config',
        'core',
        'system.yaml'
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

    # Set environment variable to control ROS logger output
    os.environ['RCUTILS_LOGGING_USE_STDOUT'] = '0' # Disable logging to stdout
    os.environ['RCUTILS_COLORIZED_OUTPUT'] = '1'   # Enable colored output

    # Generate launch description
    ld = []

    # Add Settings Creator node
    ld.append(
        Node(
            package='flychams_core',
            executable='settings_creator_node',
            name='settings_creator_node',
            output='screen',
            namespace='flychams/settings',
            parameters=[
                system_path, 
                topics_path, 
                frames_path
            ]
        )
    )

    return LaunchDescription(ld)