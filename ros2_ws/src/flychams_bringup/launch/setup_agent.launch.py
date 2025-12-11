from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory
import os
import yaml
import tempfile

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

    # Mavros parameters
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

    # Load the nodes configuration YAML file
    # Convert from PathJoinSubstitution to path string and load the file
    launch_file_path = launch_path.perform(context).strip()
    with open(launch_file_path, 'r') as f:
        launch = yaml.safe_load(f)

    # Get the node activation settings from config
    nodes = {
        # Agent setup nodes
        'mavros_manager': launch.get('mavros_manager', [True, 'info'])
    }

    # Conditionally add MavROS Manager node
    if nodes['mavros_manager'][0]:
        ld.append(
            Node(
                package='flychams_bringup',
                executable='mavros_manager_node',
                name='mavros_manager_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['mavros_manager'][1]],
                parameters=[
                    system_path,
                    topics_path,
                    frames_path,
                    bringup_path,
                    mavros_path,
                    plugin_lists_path,
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
        description='ID of the agent to setup'
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