#!/usr/bin/env python3
"""
Launch script for global instance
Can launch setup or run mode, simulated or hardware
"""

import argparse
import subprocess
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

def build_docker_exec_cmd(container_name, cmd_inside):
    docker_cmd = [
        "docker", "exec", "-it",
    ]
    
    docker_cmd.append(container_name)
    docker_cmd.append("bash")
    docker_cmd.append("-c")
    docker_cmd.append(f"'{cmd_inside}'")
    
    return " ".join(docker_cmd)

def main():
    parser = argparse.ArgumentParser(description="Launch Global Instance")
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
        'USER_NAME': user_name
    }

    # Get is_simulated parameter
    is_simulated = False if args.hardware else True

    # Launch based on mode
    if args.mode == "setup":
        print(f"Setting up global instance...")

        global_setup_cmd = (
            f"source /opt/ros/iron/setup.bash && "
            f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
            f"/home/{user_name}/FlyChams-ROS2/tools/run/setup_global.sh {is_simulated}"
        )
        docker_global_cmd = build_docker_run_cmd(
            "flychams-global", 
            global_setup_cmd, 
            base_env, 
            volumes,
            "flychams-ros2:latest"
        )
        
        # Launch global setup
        os.execv('/bin/sh', ['sh', '-c', docker_global_cmd])
            
    elif args.mode == "run":
        print(f"Running global instance...")
        
        global_run_cmd = (
            f"source /opt/ros/iron/setup.bash && "
            f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
            f"/home/{user_name}/FlyChams-ROS2/tools/run/run_global.sh {is_simulated}"
        )
        
        docker_global_exec = build_docker_exec_cmd(
            "flychams-global",
            global_run_cmd
        )
        
        # Launch global run
        os.execv('/bin/sh', ['sh', '-c', docker_global_exec])

if __name__ == "__main__":
    main()

