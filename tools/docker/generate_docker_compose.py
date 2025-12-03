import argparse
import os
import sys
import yaml
import copy
from pathlib import Path

def parse_agents_yaml(path_str):
    """Parse agents.yaml and extract agent IDs."""
    # Generate path
    path = Path(path_str)

    if not path.exists():
        raise FileNotFoundError(f"agents.yaml not found at {path}")
    
    with open(path, 'r') as f:
        agents_data = yaml.safe_load(f)
    
    if 'agents' not in agents_data:
        raise ValueError("agents.yaml does not contain 'agents' key")
    
    agents = agents_data['agents']
    if not agents:
        raise ValueError("No agents found in agents.yaml")
    
    # Extract agent IDs
    agent_ids = [agent['id'] for agent in agents if 'id' in agent]
    
    if not agent_ids:
        raise ValueError("No agent IDs found in agents.yaml")
    
    return agent_ids

def generate_compose(agent_ids):
    services = {}

    common_config = {
        'image': 'flychams-ros2:latest',
        'network_mode': 'host',
        'privileged': True,
        'environment': {
            'DISPLAY': os.environ.get('DISPLAY', ':0'),
            'QT_X11_NO_MITSHM': '1',
            'ROS_DOMAIN_ID': os.environ.get('ROS_DOMAIN_ID', '0'),
            'FASTDDS_BUILTIN_TRANSPORTS': 'UDPv4',
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
    global_config['command'] = (
        f"bash -c 'source /opt/ros/iron/setup.bash && "
        f"source /home/{os.environ.get('USER_NAME')}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
        f"/home/{os.environ.get('USER_NAME')}/FlyChams-ROS2/tools/run/setup_global.sh'"
    )

    services['flychams-global'] = global_config

    # Agent Services
    k = 0
    for agent_id in agent_ids:
        container_name = "flychams-" + agent_id
        
        agent_config = copy.deepcopy(common_config)
        agent_config['container_name'] = container_name
        
        # Add AGENT_ID env var
        agent_config['environment']['AGENT_ID'] = agent_id
        
        # PX4 SITL Command
        px4_cmd = (
            f"cd /home/{os.environ.get('USER_NAME')}/PX4-Autopilot && "
            f"PX4_SIM_HOSTNAME=172.17.0.1 PX4_SIM_MODEL=iris ./build/px4_sitl_default/bin/px4 "
            f"-i {k} "
            f"-d /home/{os.environ.get('USER_NAME')}/PX4-Autopilot/ROMFS/px4fmu_common "
            f"-s etc/init.d-posix/rcS"
        )
        
        agent_config['command'] = (
            f"bash -c 'source /opt/ros/iron/setup.bash && "
            f"source /home/{os.environ.get('USER_NAME')}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
            f"/home/{os.environ.get('USER_NAME')}/FlyChams-ROS2/tools/run/setup_agent.sh {agent_id} & "
            f"{px4_cmd}'"
        )

        services[container_name] = agent_config

        # Increment agent counter
        k = k + 1

    compose_data = {
        'services': services
    }

    return compose_data

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate docker-compose.yml for FlyChams')
    parser.add_argument('--agents-file', type=str, help='agents.yaml file')
    parser.add_argument('--output', type=str, default='docker-compose.yml', help='Output file')
    
    args = parser.parse_args()

    # Parse agent ID
    agent_ids = parse_agents_yaml(args.agents_file)
    
    # Generate Docker compose
    compose_data = generate_compose(agent_ids)
    
    with open(args.output, 'w') as f:
        yaml.dump(compose_data, f, sort_keys=False)
        
    print(f"Generated {args.output} with {len(agent_ids)} agents.")