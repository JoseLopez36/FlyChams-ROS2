#!/usr/bin/env python3
"""
Launch script for PX4 instance
"""

import os
import time
import sys
import subprocess
import argparse
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

from launchlib.launchers import PX4ContainerLauncher
from launchlib.loaders import load_environment

def main():
    # Get arguments
    parser = argparse.ArgumentParser(description="Launch PX4 instance")
    parser.add_argument("--agent-index", required=True, help="Agent index")
    args = parser.parse_args()

    # Get agent index
    agent_index = args.agent_index

    # Log
    print(f"[PX4-{agent_index}] Launching PX4 instance")

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    env_path = root_dir / '.env'

    # Load environment variables
    env = load_environment(env_path)

    # Create launcher
    launcher = PX4ContainerLauncher(agent_index, env)
    
    # Get shell command
    shell = launcher.setup(root_dir)

    # Log
    print(f"[PX4-{agent_index}] Command: {shell}")

    # Execute shell command in a separate subprocess
    process = subprocess.Popen(['bash', '-lc', shell])
    print(f"[PX4-{agent_index}] Launched setup (PID: {process.pid})")

    # Wait for process to exit
    process.wait()

    # Restore terminal
    os.system('stty sane 2>/dev/null')

if __name__ == '__main__':
    main()