#!/usr/bin/env python3
"""
Launch script for coordinator instance
"""

import os
import time
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
    parser = argparse.ArgumentParser(description="Launch coordinator instance")
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode")
    args = parser.parse_args()

    # Determine launch mode
    if args.sim and args.hardware:
        print("[COORDINATOR] Error: Cannot use both --sim and --hardware flags")
        sys.exit(1)
    elif args.sim:
        launch_mode = LaunchMode.SIMULATION
    elif args.hardware:
        launch_mode = LaunchMode.HARDWARE
    else:
        print("[COORDINATOR] Error: No launch mode specified")
        sys.exit(1)

    # Log
    print(f"[COORDINATOR] Launching coordinator instance in {launch_mode.value} mode")

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    env_path = root_dir / '.env'

    # Load environment variables
    env = load_environment(env_path)

    # Get docker username
    username = env.docker_user_name

    # Create Docker container
    container = DockerContainer(image="flychams-ros2:latest", name="flychams-COORDINATOR")

    # Create launcher
    launcher = ContainerLauncher(env, container)

    # Get is simulated argument
    is_simulated_arg = "true" if launch_mode == LaunchMode.SIMULATION else "false"

    # Create command
    cmd = f"/home/{username}/FlyChams-ROS2/tools/shell/run_coordinator.sh {is_simulated_arg}"
    
    # Get shell command
    shell = launcher.setup(cmd)

    # Log
    print(f"[COORDINATOR] Command: {shell}")

    # Execute shell command in a separate subprocess
    process = subprocess.Popen(['bash', '-lc', shell])
    print(f"[COORDINATOR] Launched coordinator instance (PID: {process.pid})")

    # Wait for process to exit
    process.wait()

    # Restore terminal
    os.system('stty sane 2>/dev/null')

if __name__ == '__main__':
    main()