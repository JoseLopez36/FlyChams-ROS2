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
    navsat_transform_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'mavros',
        'navsat_transform.yaml'
    ])
    ekf_global_path = PathJoinSubstitution([
        FindPackageShare('flychams_bringup'),
        'config',
        'mavros',
        'ekf_global.yaml'
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
        'mavros': launch.get('mavros', [True, 'info']),
        'navsat_transform': launch.get('navsat_transform', [True, 'info']),
        'ekf_global': launch.get('ekf_global', [True, 'info'])
    }

    # Conditionally add MavROS Manager node
    if nodes['mavros'][0]:
        ld.append(
            Node(
                package='flychams_bringup',
                executable='mavros_node',
                name='mavros_node',
                output='screen' if is_simulated else 'log',
                namespace='flychams/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['mavros'][1]],
                parameters=[
                    system_path,
                    topics_path,
                    frames_path,
                    mavros_path,
                    plugin_lists_path,
                    {'agent_id': agent_id},
                    {'tgt_system': 1, 'fcu_url': 'udp://:14030@172.17.0.2:14280'}
                ]
            )
        )
    
    # Create a temporary directory for modified configs (shared between nodes)
    temp_dir = tempfile.mkdtemp()
    
    # Conditionally add NavSat Transform node
    if nodes['navsat_transform'][0]:
        # Resolve the config path
        config_path_resolved = os.path.join(
            get_package_share_directory('flychams_bringup'),
            'config',
            'mavros',
            'navsat_transform.yaml'
        )

        # Create a config file with the agent_id substituted
        temp_config_path = os.path.join(temp_dir, 'navsat_transform_modified.yaml')
        
        # Read, modify and write the config file
        with open(config_path_resolved, 'r') as original_file:
            content = original_file.read()
            modified_content = content.replace('AGENTID', agent_id)
        
        with open(temp_config_path, 'w') as temp_file:
            temp_file.write(modified_content)
        
        ld.append(
            Node(
                package='robot_localization',
                executable='navsat_transform_node',
                name='navsat_transform_node',
                output='screen' if is_simulated else 'log',
                namespace='navsat/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['navsat_transform'][1]],
                parameters=[
                    temp_config_path,
                    {'use_sim_time': is_simulated}
                ],
                remappings=[
                    ('gps/fix', '/mavros/' + agent_id + '/global_position/raw/fix'),
                    ('imu/data', '/mavros/' + agent_id + '/imu/data'),
                    ('odometry/filtered', '/mavros/' + agent_id + '/global_position/local'),
                ]
            )
        )
    
    # Conditionally add EKF Global node
    if nodes['ekf_global'][0]:
        # Resolve the config path
        ekf_config_path_resolved = os.path.join(
            get_package_share_directory('flychams_bringup'),
            'config',
            'mavros',
            'ekf_global.yaml'
        )

        # Create a config file with the agent_id substituted
        temp_ekf_config_path = os.path.join(temp_dir, 'ekf_global_modified.yaml')
        
        # Read, modify and write the config file
        with open(ekf_config_path_resolved, 'r') as original_file:
            content = original_file.read()
            modified_content = content.replace('AGENTID', agent_id)
        
        with open(temp_ekf_config_path, 'w') as temp_file:
            temp_file.write(modified_content)
        
        ld.append(
            Node(
                package='robot_localization',
                executable='ekf_node',
                name='ekf_global_node',
                output='screen' if is_simulated else 'log',
                namespace='ekf/' + agent_id,
                arguments=['--ros-args', '--log-level', nodes['ekf_global'][1]],
                parameters=[
                    temp_ekf_config_path,
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