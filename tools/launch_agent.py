#!/usr/bin/env python3
"""
Launch script for agent instance
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
from launchlib.launchers import AgentContainerLauncher, AgentRemoteLauncher
from launchlib.loaders import load_environment, load_agent
from launchlib.types import LaunchMode

def main():
    # Get arguments
    parser = argparse.ArgumentParser(description="Launch global instance")
    parser.add_argument("--agent-id", required=True, help="Agent ID")
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode")
    parser.add_argument("--delay", type=float, default=1.0, help="Delay in seconds between setup and run")
    args = parser.parse_args()

    # Get agent ID
    agent_id = args.agent_id

    # Determine launch mode
    if args.sim and args.hardware:
        print(f"[{args.agent_id}] Error: Cannot use both --sim and --hardware flags")
        sys.exit(1)
    elif args.sim:
        launch_mode = LaunchMode.SIMULATION
    elif args.hardware:
        launch_mode = LaunchMode.HARDWARE
    else:
        print(f"[{agent_id}] Error: No launch mode specified")
        sys.exit(1)

    # Log
    print(f"[{agent_id}] Launching agent instance in {launch_mode.value} mode")

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    env_path = root_dir / '.env'
    mission_path = root_dir / 'config' / 'mission.yaml'

    # Load environment variables
    env = load_environment(env_path)
    agent = load_agent(agent_id, mission_path)

    # Get is simulated argument
    is_simulated_arg = "true" if launch_mode == LaunchMode.SIMULATION else "false"

    # Create Docker container
    container = DockerContainer(image="flychams-ros2:latest", name=f"flychams-{agent_id}")

    # SIMULATION mode: Setup and run in Docker container
    if launch_mode == LaunchMode.SIMULATION:
        # Create launcher
        launcher = AgentContainerLauncher(agent_id, env, container)
        
        # Create command
        setup_cmd = "setup_agent.sh" + " " + is_simulated_arg
        
        # Get setup shell command
        setup_shell = launcher.setup(setup_cmd)

        # Log
        print(f"[{agent_id}] Setup command: {setup_shell}")

        # Execute setup shell command in a separate subprocess
        setup_process = subprocess.Popen(['bash', '-lc', setup_shell])
        print(f"[{agent_id}] Launched setup (PID: {setup_process.pid})")

        # Wait a delay for container to be ready
        time.sleep(args.delay)

        # Create command
        run_cmd = "run_agent.sh" + " " + is_simulated_arg
        
        # Get run shell command
        run_shell = launcher.run(run_cmd)

        # Log
        print(f"[{agent_id}] Run command: {run_shell}")

        # Execute run shell command in a separate subprocess and wait for it
        run_process = subprocess.Popen(['bash', '-lc', run_shell])
        print(f"[{agent_id}] Launched run command (PID: {run_process.pid})")

        # Wait for run process to exit
        run_process.wait()

        # Restore terminal
        os.system('stty sane 2>/dev/null')

    # HARDWARE mode: Setup and run on remote onboard computer
    elif launch_mode == LaunchMode.HARDWARE:
        # Create launcher
        launcher = AgentRemoteLauncher(agent_id, agent.ssh)
        
        # Create command
        setup_cmd = "setup_agent.sh" + " " + is_simulated_arg
        
        # Get setup shell command
        setup_shell = launcher.setup(setup_cmd)

        # Log
        print(f"[{agent_id}] Setup command: {setup_shell}")

        # Execute setup shell command in a separate subprocess
        setup_process = subprocess.Popen(['bash', '-lc', setup_shell])
        print(f"[{agent_id}] Launched setup (PID: {setup_process.pid})")

        # Wait a delay for container to be ready
        time.sleep(args.delay)

        # Create command
        run_cmd = "run_agent.sh" + " " + is_simulated_arg
        
        # Get run shell command
        run_shell = launcher.run(run_cmd)

        # Log
        print(f"[{agent_id}] Run command: {run_shell}")

        # Execute run shell command in a separate subprocess and wait for it
        run_process = subprocess.Popen(['bash', '-lc', run_shell])
        print(f"[{agent_id}] Launched run command (PID: {run_process.pid})")

        # Wait for run process to exit
        run_process.wait()

        # Restore terminal
        os.system('stty sane 2>/dev/null') 

if __name__ == '__main__':
    main()