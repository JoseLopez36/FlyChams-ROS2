#!/usr/bin/env python3
"""
Run operator interface node
"""

import os
import sys
import subprocess
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

from launchlib.docker import DockerContainer
from launchlib.launchers import ContainerLauncher
from launchlib.loaders import load_environment

def main():
    # Log
    print(f"[OPERATOR-INTERFACE] Running operator interface node")

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
    container = DockerContainer(image="flychams-ros2:latest", name="flychams-OPERATOR-INTERFACE")

    # Setup X11 authorization
    container.setup_auth()

    # Create launcher
    launcher = ContainerLauncher(env, container)
    
    # Create command
    cmd = f"/home/{username}/FlyChams-ROS2/tools/shell/run_operator_interface.sh"
    
    # Get shell command
    shell = launcher.setup(cmd)

    # Log
    print(f"[OPERATOR-INTERFACE] Command: {shell}")

    # Execute shell command in a separate subprocess
    process = subprocess.Popen(['bash', '-lc', shell])
    print(f"[OPERATOR-INTERFACE] Launched operator interface (PID: {process.pid})")

    # Wait for process to exit
    process.wait()

    # Restore terminal
    os.system('stty sane 2>/dev/null')

if __name__ == '__main__':
    main()