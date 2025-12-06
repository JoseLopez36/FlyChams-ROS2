#!/usr/bin/env python3
import os
import sys
import subprocess
import argparse
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

def main():
    # Get arguments
    parser = argparse.ArgumentParser(description='Run build instance')
    parser.add_argument('--regenerate-airsim', action='store_true', help='Regenerate AirSim wrappers')
    args = parser.parse_args()

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent.parent
    config_env_path = root_dir / 'docker' / 'config.env'
    
    # Load environment variables
    if config_env_path.exists():
        load_env_file(config_env_path)
    
    user_name = os.environ.get('USER_NAME')
    flychams_ros2_path = os.environ.get('FLYCHAMS_ROS2_PATH')
    flychams_airsim_path = os.environ.get('FLYCHAMS_AIRSIM_PATH')
    flychams_px4_path = os.environ.get('FLYCHAMS_PX4_PATH')

    # Get regenerate argument
    regen_arg = "--regenerate-airsim" if args.regenerate_airsim else ""
    
    print("Starting build instance...")
    print(f"Mounting:\n  - {flychams_ros2_path}\n  - {flychams_airsim_path}\n  - {flychams_px4_path}")
    
    docker_cmd = [
        'docker', 'run', '--rm', '-it',
        '--name', 'flychams-build',
        '--network', 'host',
        '--privileged',
        '-e', f"DISPLAY={os.environ.get('DISPLAY', ':0')}",
        '-e', 'QT_X11_NO_MITSHM=1',
        '-e', f"FLYCHAMS_PATH=/home/{user_name}/FlyChams-ROS2",
        '-e', f"AIRSIM_PATH=/home/{user_name}/FlyChams-Cosys-AirSim",
        '-e', f"PX4_PATH=/home/{user_name}/PX4-Autopilot",
        '-v', '/tmp/.X11-unix:/tmp/.X11-unix',
        '-v', f"{flychams_ros2_path}:/home/{user_name}/FlyChams-ROS2",
        '-v', f"{flychams_airsim_path}:/home/{user_name}/FlyChams-Cosys-AirSim",
        '-v', f"{flychams_px4_path}:/home/{user_name}/PX4-Autopilot",
        'flychams-ros2:latest',
        'bash', '-c',
        f"source /opt/ros/iron/setup.bash && /home/{user_name}/FlyChams-ROS2/tools/build/setup.sh {regen_arg}"
    ]
    
    try:
        subprocess.run(docker_cmd, check=True)
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)

if __name__ == '__main__':
    main()