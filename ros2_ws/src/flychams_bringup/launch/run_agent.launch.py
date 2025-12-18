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

    # ============= CONTROL NODES =============
    # Conditionally add Drone Frames node
    if is_enabled('drone_frames'):
        ld.append(
            Node(
                package='flychams_control',
                executable='drone_frames_node',
                name='drone_frames_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', log_level('drone_frames')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    mission_params_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Drone State node
    if is_enabled('drone_state'):
        ld.append(
            Node(
                package='flychams_control',
                executable='drone_state_node',
                name='drone_state_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', log_level('drone_state')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    mission_params_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )
        
    # Conditionally add Drone Control node
    if is_enabled('drone_control'):
        ld.append(
            Node(
                package='flychams_control',
                executable='drone_control_node',
                name='drone_control_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', log_level('drone_control')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    mission_params_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Camera Frames node
    if is_enabled('camera_frames'):
        ld.append(
            Node(
                package='flychams_control',
                executable='camera_frames_node',
                name='camera_frames_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', log_level('camera_frames')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    mission_params_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )
        
    # Conditionally add Camera Control node
    if is_enabled('camera_control'):
        ld.append(
            Node(
                package='flychams_control',
                executable='camera_control_node',
                name='camera_control_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', log_level('camera_control')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    control_path,
                    mission_params_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # ============= COORDINATION NODES =============
    # Conditionally add Agent Positioning node
    if is_enabled('agent_positioning'):
        ld.append(
            Node(
                package='flychams_coordination',
                executable='agent_positioning_node',
                name='agent_positioning_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', log_level('agent_positioning')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    mission_params_path,
                    {'agent_id': agent_id},
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Agent Tracking node
    if is_enabled('agent_tracking'):
        ld.append(
            Node(
                package='flychams_coordination',
                executable='agent_tracking_node',
                name='agent_tracking_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', log_level('agent_tracking')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    mission_params_path,
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

