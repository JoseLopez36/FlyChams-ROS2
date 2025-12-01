from launch import LaunchDescription
from launch_ros.actions import Node
from launch import LaunchContext
from launch.actions import DeclareLaunchArgument
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare
import os
import yaml

def generate_launch_description():
    # Declare arguments
    agent_id_arg = DeclareLaunchArgument(
        'agent_id',
        default_value='agent_0',
        description='ID of the agent to control'
    )
    
    agent_id = LaunchConfiguration('agent_id')

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

    # Set environment variable to control ROS logger output
    os.environ['RCUTILS_LOGGING_USE_STDOUT'] = '1' # Enable logging to stdout
    os.environ['RCUTILS_COLORIZED_OUTPUT'] = '1'   # Enable colored output

    # Generate launch description
    ld = [agent_id_arg]

    # Load the nodes configuration YAML file
    # Convert from PathJoinSubstitution to path string and load the file
    launch_file_path = launch_path.perform(LaunchContext()).strip()
    with open(launch_file_path, 'r') as f:
        launch = yaml.safe_load(f)
    
    # Get the node activation settings from config
    nodes = {
        # Control nodes (Agent)
        'drone_state': launch.get('drone_state', [True, 'info']),
        'drone_control': launch.get('drone_control', [True, 'info']),
        'camera_control': launch.get('camera_control', [True, 'info']),
        # Coordination nodes (Agent)
        'agent_positioning': launch.get('agent_positioning', [True, 'info']),
        'agent_tracking': launch.get('agent_tracking', [True, 'info']),
    }

    # ============= CONTROL NODES =============
    # Conditionally add Drone State node
    if nodes['drone_state'][0]:
        ld.append(
            Node(
                package='flychams_control',
                executable='drone_state_node',
                name='drone_state_node',
                output='screen',
                namespace='flychams',
                arguments=['--ros-args', '--log-level', nodes['drone_state'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'use_sim_time': True},
                    {'assigned_agent_id': agent_id}
                ]
            )
        )
        
    # Conditionally add Drone Control node
    if nodes['drone_control'][0]:
        ld.append(
            Node(
                package='flychams_control',
                executable='drone_control_node',
                name='drone_control_node',
                output='screen',
                namespace='flychams',
                arguments=['--ros-args', '--log-level', nodes['drone_control'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'use_sim_time': True},
                    {'assigned_agent_id': agent_id}
                ]
            )
        )

    # Conditionally add Camera Control node
    if nodes['camera_control'][0]:
        ld.append(
            Node(
                package='flychams_control',
                executable='camera_control_node',
                name='camera_control_node',
                output='screen',
                namespace='flychams',
                arguments=['--ros-args', '--log-level', nodes['camera_control'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'use_sim_time': True},
                    {'assigned_agent_id': agent_id}
                ]
            )
        )

    # ============= COORDINATION NODES =============
    # Conditionally add Agent Positioning node
    if nodes['agent_positioning'][0]:
        ld.append(
            Node(
                package='flychams_coordination',
                executable='agent_positioning_node',
                name='agent_positioning_node',
                output='screen',
                namespace='flychams',
                arguments=['--ros-args', '--log-level', nodes['agent_positioning'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    {'use_sim_time': True},
                    {'assigned_agent_id': agent_id}
                ]
            )
        )

    # Conditionally add Agent Tracking node
    if nodes['agent_tracking'][0]:
        ld.append(
            Node(
                package='flychams_coordination',
                executable='agent_tracking_node',
                name='agent_tracking_node',
                output='screen',
                namespace='flychams',
                arguments=['--ros-args', '--log-level', nodes['agent_tracking'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    {'use_sim_time': True},
                    {'assigned_agent_id': agent_id}
                ]
            )
        )

    return LaunchDescription(ld)

