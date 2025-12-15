#!/usr/bin/env python3
"""
Launch script for visualization (RViz or PlotJuggler)
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
    parser = argparse.ArgumentParser(description='Launch visualization instance')
    parser.add_argument('--plotjuggler', action='store_true', help='Run PlotJuggler instead of RViz')
    args = parser.parse_args()

    # Determine launch mode
    launch_mode = LaunchMode.SIMULATION

    # Log
    print(f"[VISUALIZATION] Launching visualization instance in {launch_mode.value} mode")

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    env_path = root_dir / 'docker' / 'config.env'

    # Load environment variables
    env = load_environment(env_path)

    # Create Docker container
    container = DockerContainer(image="flychams-ros2:latest", name="flychams-VISUALIZATION")

    # Setup X11 authorization
    container.setup_auth()

    # Create launcher
    launcher = ContainerLauncher(env, container)
    
    # Create command
    if args.plotjuggler:
        setup_cmd = "run_plotjuggler.sh"
    else:
        setup_cmd = "run_rviz.sh"
    
    # Get setup shell command
    setup_shell = launcher.setup(setup_cmd)

    # Log
    print(f"[VISUALIZATION] Setup command: {setup_shell}")

    # Execute setup shell command in a separate subprocess
    setup_process = subprocess.Popen(['bash', '-lc', setup_shell])
    print(f"[VISUALIZATION] Launched visualization instance (PID: {setup_process.pid})")

    # Wait for setup process to exit
    setup_process.wait()

    # Restore terminal
    os.system('stty sane 2>/dev/null')

if __name__ == '__main__':
    main()