#!/usr/bin/env python3
"""
Launch script for simulation instance
"""

import os
import sys
import subprocess
import shlex
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

from launchlib.docker import DockerContainer
from launchlib.launchers import ContainerLauncher
from launchlib.loaders import load_environment

def main():
    # Log
    print(f"[SIMULATION] Launching simulation instance")

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    env_path = root_dir / '.env'

    # Load environment variables
    env = load_environment(env_path)

    # Get docker username
    username = env.docker_user_name

    # Launch Unreal Engine 5
    ue5_cmd = f"bash {shlex.quote(str(root_dir / 'tools' / 'shell' / 'run_ue5.sh'))}"
    ue5_process = subprocess.Popen(['bash', '-lc', ue5_cmd])
    print(f"[SIMULATION] Launched Unreal Engine 5 (PID: {ue5_process.pid})")

    # Create Docker container
    container = DockerContainer(image="flychams-ros2:latest", name="flychams-SIMULATION")

    # Create launcher
    launcher = ContainerLauncher(env, container)
    
    # Create command
    cmd = f"/home/{username}/FlyChams-ROS2/tools/shell/run_simulation.sh"
    
    # Get shell command
    shell = launcher.setup(cmd)

    # Log
    print(f"[SIMULATION] Command: {shell}")

    # Execute shell command in a separate subprocess
    process = subprocess.Popen(['bash', '-lc', shell])
    print(f"[SIMULATION] Launched simulation instance (PID: {process.pid})")

    # Wait for process to exit
    process.wait()

    # Restore terminal
    os.system('stty sane 2>/dev/null')

if __name__ == '__main__':
    main()