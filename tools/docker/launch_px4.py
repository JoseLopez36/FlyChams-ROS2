#!/usr/bin/env python3
"""
Launch script for PX4 SITL instances (simulation only)
"""

import argparse
import os
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

def main():
    parser = argparse.ArgumentParser(description="Launch PX4 SITL Instance (Simulation Only)")
    parser.add_argument("--agent-id", required=True, help="Agent ID")
    parser.add_argument("--agent-index", type=int, required=True, help="Agent index for PX4 SITL instance number")
    args = parser.parse_args()

    # Get script directory
    script_dir = Path(__file__).resolve().parent
    root_dir = script_dir.parent.parent
    config_env_path = root_dir / 'docker' / 'config.env'

    # Load environment variables
    if config_env_path.exists():
        load_env_file(config_env_path)
    user_name = os.environ.get('USER_NAME')
    flychams_px4_path = os.environ.get('FLYCHAMS_PX4_PATH')
    
    if not user_name or not flychams_px4_path:
        print("Error: USER_NAME or FLYCHAMS_PX4_PATH not set in config.env")
        sys.exit(1)
    
    # Get Docker config
    volumes = {
        '/tmp/.X11-unix': '/tmp/.X11-unix',
        flychams_px4_path: f"/home/{user_name}/PX4-Autopilot"
    }
    base_env = {
        'DISPLAY': os.environ.get('DISPLAY', ':0'),
        'QT_X11_NO_MITSHM': '1',
        'PX4_PATH': f"/home/{user_name}/PX4-Autopilot",
        'USER_NAME': user_name,
        'AGENT_ID': args.agent_id
    }
    
    container_name = f"flychams-px4-{args.agent_id}"
    
    print(f"Launching PX4 SITL for agent {args.agent_id} (instance {args.agent_index})...")
    
    # Build PX4 SITL command
    px4_cmd = (
        f"PX4_SIM_HOSTNAME=172.17.0.1 PX4_SIM_MODEL=iris "
        f"/home/{user_name}/PX4-Autopilot/build/px4_sitl_default/bin/px4 "
        f"-i {args.agent_index} "
        f"-d /home/{user_name}/PX4-Autopilot/ROMFS/px4fmu_common "
        f"-s etc/init.d-posix/rcS"
    )
    
    docker_px4_cmd = build_docker_run_cmd(
        container_name,
        px4_cmd,
        base_env,
        volumes,
        "flychams-ros2:latest"
    )
    
    # Launch PX4 SITL
    os.execv('/bin/sh', ['sh', '-c', docker_px4_cmd])

if __name__ == "__main__":
    main()

