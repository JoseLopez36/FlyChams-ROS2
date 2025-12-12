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
    bringup_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'bringup.yaml'
    ])
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
    perception_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'perception.yaml'
    ])
    simulation_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'package',
        'simulation.yaml'
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

    # ============= PERCEPTION NODES =============
    # Conditionally add Target Clustering node
    if is_enabled('target_clustering'):
        ld.append(
            Node(
                package='flychams_perception',
                executable='target_clustering_node',
                name='target_clustering_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('target_clustering')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    perception_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Cluster Analysis node
    if is_enabled('cluster_analysis'):
        ld.append(
            Node(
                package='flychams_perception',
                executable='cluster_analysis_node',
                name='cluster_analysis_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('cluster_analysis')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    perception_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # ============= COORDINATION NODES =============
    # Conditionally add Agent Assignment node
    if is_enabled('agent_assignment'):
        ld.append(
            Node(
                package='flychams_coordination',
                executable='agent_assignment_node',
                name='agent_assignment_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('agent_assignment')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Agent Analysis node
    if is_enabled('agent_analysis'):
        ld.append(
            Node(
                package='flychams_coordination',
                executable='agent_analysis_node',
                name='agent_analysis_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('agent_analysis')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    coordination_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # ============= SIMULATION NODES =============
    # Conditionally add GUI Manager node
    if is_enabled('gui_manager'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='gui_manager_node',
                name='gui_manager_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('gui_manager')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Metrics Factory node
    if is_enabled('metrics_factory'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='metrics_factory_node',
                name='metrics_factory_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('metrics_factory')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Marker Factory node
    if is_enabled('marker_factory'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='marker_factory_node',
                name='marker_factory_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('marker_factory')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Target State node
    if is_enabled('target_state'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='target_state_node',
                name='target_state_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('target_state')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
                    mission_params_path,
                    {'use_sim_time': is_simulated}
                ]
            )
        )

    # Conditionally add Target Control node
    if is_enabled('target_control'):
        ld.append(
            Node(
                package='flychams_simulation',
                executable='target_control_node',
                name='target_control_node',
                output='screen',
                namespace='flychams/global',
                arguments=['--ros-args', '--log-level', log_level('target_control')],
                parameters=[
                    system_path, 
                    topics_path, 
                    frames_path, 
                    simulation_path,
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
