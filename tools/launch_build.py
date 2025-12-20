#!/usr/bin/env python3
"""
Launch script for build instance
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

from launchlib.docker import DockerContainer
from launchlib.launchers import ContainerLauncher
from launchlib.loaders import load_environment
from launchlib.types import LaunchMode

def main():
    # Get arguments
    parser = argparse.ArgumentParser(description='Launch build instance')
    parser.add_argument('--build-on-host', action='store_true', help='Build on host machine')
    parser.add_argument('--build-ros2', action='store_true', help='Build ROS2 workspace')
    parser.add_argument('--build-airsim', action='store_true', help='Build AirSim dependencies')
    parser.add_argument('--generate-settings', action='store_true', help='Generate settings')
    parser.add_argument('-j', type=int, default=2, help='Number of threads to use for ROS2 building')
    args = parser.parse_args()

    # Log
    print(f"[BUILD] Launching build instance")

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    env_path = root_dir / '.env'

    # Load environment variables
    env = load_environment(env_path)

    if args.build_on_host:
        # Create command
        if args.build_ros2:
            if args.j > 0:
                setup_cmd = f"build_ros2_ws.sh -j {args.j}"
            else:
                setup_cmd = "build_ros2_ws.sh"
        elif args.build_airsim:
            setup_cmd = "build_airsim.sh"
        elif args.generate_settings:
            setup_cmd = "create_settings.sh"
        else:
            print("[BUILD] Error: No build option specified")
            sys.exit(1)
        
        # Execute shell command
        os.execv(f"{env.flychams_ros2_path}/tools/shell/{setup_cmd}", ["bash", "-lc", setup_cmd])
        sys.exit(0)

    # Load environment variables
    env = load_environment(env_path)

    # Create Docker container
    container = DockerContainer(image="flychams-ros2:latest", name="flychams-BUILD")

    # Create launcher
    launcher = ContainerLauncher(env, container)
    
    # Create command
    if args.build_ros2:
        if args.j > 0:
            setup_cmd = f"build_ros2_ws.sh -j {args.j}"
        else:
            setup_cmd = "build_ros2_ws.sh"
    elif args.build_airsim:
        setup_cmd = "build_airsim.sh"
    elif args.generate_settings:
        setup_cmd = "create_settings.sh"
    else:
        print("[BUILD] Error: No build option specified")
        sys.exit(1)
    
    # Get setup shell command
    setup_shell = launcher.setup(setup_cmd)

    # Log
    print(f"[BUILD] Setup command: {setup_shell}")

    # Execute setup shell command in a separate subprocess and wait for it
    setup_process = subprocess.Popen(['bash', '-lc', setup_shell])
    print(f"[BUILD] Launched build instance (PID: {setup_process.pid})")

    # Wait for setup process to exit
    setup_process.wait()

    # Restore terminal
    os.system('stty sane 2>/dev/null')

if __name__ == '__main__':
    main()