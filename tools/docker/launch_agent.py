#!/usr/bin/env python3
"""
Launch script for agent instances
Can launch setup or run mode, simulated (local Docker) or hardware (remote SSH)
"""

import argparse
import subprocess
import os
import yaml
import sys
from pathlib import Path

def load_env_file(env_file):
    with open(env_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            if '=' in line:
                key, value = line.split('=', 1)
                value = os.path.expandvars(value)
                os.environ[key] = value

def build_docker_run_cmd(name, cmd_inside, env_vars, volumes, image):
    docker_cmd = [
        "docker", "run", "--rm",
        "--name", name,
        "--net=host",
        "--privileged",
    ]
    
    # Add environment variables
    for k, v in env_vars.items():
        docker_cmd.extend(["-e", f"{k}={v}"])
        
    # Add volumes
    for host_path, container_path in volumes.items():
        docker_cmd.extend(["-v", f"{host_path}:{container_path}"])
        
    docker_cmd.append(image)
    docker_cmd.append("bash")
    docker_cmd.append("-c")
    docker_cmd.append(f"'{cmd_inside}'")
    
    return " ".join(docker_cmd)

def build_docker_exec_cmd(container_name, cmd_inside):
    docker_cmd = [
        "docker", "exec", "-it",
    ]
    
    docker_cmd.append(container_name)
    docker_cmd.append("bash")
    docker_cmd.append("-c")
    docker_cmd.append(f"'{cmd_inside}'")
    
    return " ".join(docker_cmd)

def build_ssh_cmd(remote_host, cmd):
    user = remote_host['user']
    hostname = remote_host['hostname']
    return f"ssh {user}@{hostname} '{cmd}'"

def get_agent_ssh_config(agent_id, mission_yaml_path):
    path = Path(mission_yaml_path)
    with open(path, 'r') as f:
        mission_data = yaml.safe_load(f)
    
    agents = mission_data['/**']['ros__parameters']['agents']
    if agent_id in agents:
        agent = agents[agent_id]
        if isinstance(agent, dict) and 'ssh' in agent:
            return {
                'user': agent.get('ssh').get('user'),
                'hostname': agent.get('ssh').get('hostname')
            }
    return None

def main():
    parser = argparse.ArgumentParser(description="Launch Agent Instance")
    parser.add_argument("--agent-id", required=True, help="Agent ID")
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode")
    parser.add_argument("--mode", required=True, choices=["setup", "run"], help="Launch mode: setup or run")
    args = parser.parse_args()

    # Determine launch mode
    if args.sim and args.hardware:
        print("Error: Cannot use both --sim and --hardware flags")
        sys.exit(1)

    # Default to simulation mode if neither flag is set
    launch_mode = "hardware" if args.hardware else "simulation"
    print(f"Launch mode: {launch_mode}")
    
    # Get script directory
    script_dir = Path(__file__).resolve().parent
    root_dir = script_dir.parent.parent
    config_env_path = root_dir / 'docker' / 'config.env'
    mission_yaml_path = root_dir / 'config' / 'mission.yaml'

    # Get is_simulated parameter
    is_simulated = False if launch_mode == "hardware" else True
    
    if launch_mode == "hardware":
        # Hardware mode: launch via SSH on remote Jetson
        # Get onboard computer parameters
        user_name = os.environ.get('USER', 'jetson')
        flychams_ros2_path = f"/home/{user_name}/FlyChams-ROS2"
        
        # Get remote host SSH configuration
        remote_host = get_agent_ssh_config(args.agent_id, mission_yaml_path)
        
        if not remote_host:
            print(f"Error: Agent {args.agent_id} missing SSH configuration for hardware mode")
            sys.exit(1)
        
        if args.mode == "setup":
            print(f"Setting up agent {args.agent_id} on {remote_host['hostname']}...")
            
            # Build command to run directly on Jetson
            agent_setup_cmd = (
                f"source /opt/ros/iron/setup.bash && "
                f"source {flychams_ros2_path}/ros2_ws/install/setup.bash && "
                f"{flychams_ros2_path}/tools/run/setup_agent.sh {args.agent_id} {is_simulated}"
            )
            
            ssh_cmd = build_ssh_cmd(remote_host, agent_setup_cmd)
            os.execv('/bin/sh', ['sh', '-c', ssh_cmd])
            
        elif args.mode == "run":
            print(f"Running agent {args.agent_id} on {remote_host['hostname']}...")
            
            # Build command to run directly on Jetson
            agent_run_cmd = (
                f"source /opt/ros/iron/setup.bash && "
                f"source {flychams_ros2_path}/ros2_ws/install/setup.bash && "
                f"{flychams_ros2_path}/tools/run/run_agent.sh {args.agent_id} {is_simulated}"
            )
            
            ssh_cmd = build_ssh_cmd(remote_host, agent_run_cmd)
            os.execv('/bin/sh', ['sh', '-c', ssh_cmd])

    elif launch_mode == "simulation":
        # Simulation mode: launch in Docker container locally
        # Load environment variables
        if config_env_path.exists():
            load_env_file(config_env_path)
        user_name = os.environ.get('USER_NAME')
        flychams_ros2_path = os.environ.get('FLYCHAMS_ROS2_PATH')
        flychams_airsim_path = os.environ.get('FLYCHAMS_AIRSIM_PATH')
        flychams_px4_path = os.environ.get('FLYCHAMS_PX4_PATH')
        # Get Docker config
        volumes = {
            '/tmp/.X11-unix': '/tmp/.X11-unix',
            flychams_ros2_path: f"/home/{user_name}/FlyChams-ROS2",
            flychams_airsim_path: f"/home/{user_name}/FlyChams-Cosys-AirSim",
            flychams_px4_path: f"/home/{user_name}/PX4-Autopilot"
        }
        base_env = {
            'DISPLAY': os.environ.get('DISPLAY', ':0'),
            'QT_X11_NO_MITSHM': '1',
            'ROS_DOMAIN_ID': os.environ.get('ROS_DOMAIN_ID', '0'),
            'FASTDDS_BUILTIN_TRANSPORTS': 'UDPv4',
            'FLYCHAMS_PATH': f"/home/{user_name}/FlyChams-ROS2",
            'AIRSIM_PATH': f"/home/{user_name}/FlyChams-Cosys-AirSim",
            'PX4_PATH': f"/home/{user_name}/PX4-Autopilot",
            'USER_NAME': user_name,
            'AGENT_ID': args.agent_id
        }
        
        container_name = f"flychams-{args.agent_id}"
        
        if args.mode == "setup":
            print(f"Setting up agent {args.agent_id} on simulation mode...")
            
            # Build setup command
            agent_setup_cmd = (
                f"source /opt/ros/iron/setup.bash && "
                f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
                f"/home/{user_name}/FlyChams-ROS2/tools/run/setup_agent.sh {args.agent_id} {is_simulated}"
            )
            
            docker_agent_cmd = build_docker_run_cmd(
                container_name,
                agent_setup_cmd,
                base_env,
                volumes,
                "flychams-ros2:latest"
            )
            
            # Launch agent setup
            os.execv('/bin/sh', ['sh', '-c', docker_agent_cmd])
                
        elif args.mode == "run":
            print(f"Running agent {args.agent_id} on simulation mode...")
            
            agent_run_cmd = (
                f"source /opt/ros/iron/setup.bash && "
                f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
                f"/home/{user_name}/FlyChams-ROS2/tools/run/run_agent.sh {args.agent_id} {is_simulated}"
            )
            
            docker_agent_exec = build_docker_exec_cmd(
                container_name,
                agent_run_cmd
            )
            
            # Launch agent run
            os.execv('/bin/sh', ['sh', '-c', docker_agent_exec])

if __name__ == "__main__":
    main()

