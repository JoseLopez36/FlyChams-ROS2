from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import OpaqueFunction
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
import os

def launch_setup(context, *args, **kwargs):
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
        'operator.yaml'
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

    ld.append(
        Node(
            package='flychams_operator',
            executable='operator_interface_node.py',
            name='operator_interface_node',
            output='screen',
            namespace='flychams/operator',
            parameters=[
                system_path, 
                topics_path, 
                frames_path, 
                dashboard_path,
                mission_path
            ]
        )
    )

    return ld

def generate_launch_description():
    return LaunchDescription([
        OpaqueFunction(function=launch_setup)
    ])
