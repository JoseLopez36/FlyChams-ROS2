from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare
import os
import yaml

def launch_setup(context, *args, **kwargs):
    # Get agent_id value from LaunchConfiguration
    agent_id = LaunchConfiguration('agent_id').perform(context)
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
    ld = []

    # Load the nodes configuration YAML file
    # Convert from PathJoinSubstitution to path string and load the file
    launch_file_path = launch_path.perform(context).strip()
    with open(launch_file_path, 'r') as f:
        launch = yaml.safe_load(f)
    
    # Get the node activation settings from config
    nodes = {
        # Control nodes (Agent)
        'drone_frames': launch.get('drone_frames', [True, 'info']),
        'drone_state': launch.get('drone_state', [True, 'info']),
        'drone_control': launch.get('drone_control', [True, 'info']),
        'camera_frames': launch.get('camera_frames', [True, 'info']),
        'camera_control': launch.get('camera_control', [True, 'info']),
        # Coordination nodes (Agent)
        'agent_positioning': launch.get('agent_positioning', [True, 'info']),
        'agent_tracking': launch.get('agent_tracking', [True, 'info']),
    }

    # ============= CONTROL NODES =============
    # Conditionally add Drone Frames node
    if nodes['drone_frames'][0]:
        ld.append(
            Node(
                package='flychams_control',
                executable='drone_frames_node',
                name='drone_frames_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['drone_frames'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Drone State node
    if nodes['drone_state'][0]:
        ld.append(
            Node(
                package='flychams_control',
                executable='drone_state_node',
                name='drone_state_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['drone_state'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
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
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['drone_control'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Camera Frames node
    if nodes['camera_frames'][0]:
        ld.append(
            Node(
                package='flychams_control',
                executable='camera_frames_node',
                name='camera_frames_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['camera_frames'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
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
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['camera_control'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
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
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['agent_positioning'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
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
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['agent_tracking'][1]],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    return ld

def generate_launch_description():
    # Declare arguments
    agent_id_arg = DeclareLaunchArgument(
        'agent_id',
        default_value='',
        description='ID of the agent to run'
    )
    is_simulated_arg = DeclareLaunchArgument(
        'is_simulated',
        default_value='False',
        description='Whether the agent is simulated'
    )

    return LaunchDescription([
        agent_id_arg,
        is_simulated_arg,
        OpaqueFunction(function=launch_setup)
    ])

