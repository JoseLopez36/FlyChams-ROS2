import argparse
import os
import yaml
import copy

def generate_compose(agents):
    services = {}

    common_config = {
        'image': 'flychams-ros2:latest',
        'network_mode': 'host',
        'privileged': True,
        'environment': {
            'DISPLAY': os.environ.get('DISPLAY', ':0'),
            'QT_X11_NO_MITSHM': '1',
            'ROS_DOMAIN_ID': os.environ.get('ROS_DOMAIN_ID', '0'),
            'FLYCHAMS_PATH': f'/home/{os.environ.get("USER_NAME")}/FlyChams-ROS2',
            'AIRSIM_PATH': f'/home/{os.environ.get("USER_NAME")}/FlyChams-Cosys-AirSim',
            'PX4_PATH': f'/home/{os.environ.get("USER_NAME")}/PX4-Autopilot'
        },
        'volumes': [
            '/tmp/.X11-unix:/tmp/.X11-unix',
            f"{os.environ.get('FLYCHAMS_ROS2_PATH')}:/home/{os.environ.get('USER_NAME')}/FlyChams-ROS2",
            f"{os.environ.get('FLYCHAMS_AIRSIM_PATH')}:/home/{os.environ.get('USER_NAME')}/FlyChams-Cosys-AirSim",
            f"{os.environ.get('FLYCHAMS_PX4_PATH')}:/home/{os.environ.get('USER_NAME')}/PX4-Autopilot"
        ],
        'stdin_open': True,
        'tty': True
    }

    # Global Service
    global_config = copy.deepcopy(common_config)
    global_config['container_name'] = 'flychams-global'
    # Start command for global
    # We need to make sure we source the workspace before running
    global_config['command'] = f"bash -c 'source /opt/ros/iron/setup.bash && source /home/{os.environ.get('USER_NAME')}/FlyChams-ROS2/ros2_ws/install/setup.bash && /home/{os.environ.get('USER_NAME')}/FlyChams-ROS2/tools/run/setup.sh'"
    services['flychams-global'] = global_config

    # Agent Services
    for k in range(agents):
        container_name = f"flychams-agent-{k}"
        
        agent_config = copy.deepcopy(common_config)
        agent_config['container_name'] = container_name
        
        # Add AGENT_ID env var
        agent_config['environment']['AGENT_ID'] = f"agent_{k}"
        
        # Command
        agent_config['command'] = f"bash -c 'source /opt/ros/iron/setup.bash && source /home/{os.environ.get('USER_NAME')}/FlyChams-ROS2/ros2_ws/install/setup.bash'"
        
        services[container_name] = agent_config

    compose_data = {
        'services': services
    }

    return compose_data

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate docker-compose.yml for FlyChams')
    parser.add_argument('--agents', type=int, default=1, help='Number of agents')
    parser.add_argument('--output', type=str, default='docker-compose.yml', help='Output file')
    
    args = parser.parse_args()
    
    compose_data = generate_compose(args.agents)
    
    with open(args.output, 'w') as f:
        yaml.dump(compose_data, f, sort_keys=False)
        
    print(f"Generated {args.output} with {args.agents} agents.")

