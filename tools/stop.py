#!/usr/bin/env python3

import os
import time
import threading
import libtmux
import sys
import subprocess
import argparse
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

from launchlib.types import LaunchMode, AgentSSH
from launchlib.loaders import load_agents

def stop_containers(filter_name):
    subprocess.run(f"docker ps -q --filter name={filter_name} | xargs -r docker stop", shell=True)

def stop_processes(agent_id, ssh: AgentSSH):
    cmd = (
        f"pkill -f 'run_agent.launch.py.*agent_id:={agent_id}' || true"
    )
    ssh_cmd = f"ssh {ssh.user}@{ssh.hostname} '{cmd}'"
    subprocess.run(ssh_cmd, shell=True)
    print(f"[STOP] Stopped ROS2 processes for agent {agent_id} on {ssh.user}@{ssh.hostname}")

def main():
    parser = argparse.ArgumentParser(description="Stop FlyChams")
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode")
    args = parser.parse_args()

    # Determine launch mode
    if args.sim and args.hardware:
        print("[STOP] Error: Cannot use both --sim and --hardware flags")
        sys.exit(1)
    elif args.sim:
        launch_mode = LaunchMode.SIMULATION
    elif args.hardware:
        launch_mode = LaunchMode.HARDWARE
    else:
        print("[STOP] Error: No launch mode specified")
        sys.exit(1)
    
    # Log
    print(f"[STOP] Stopping FlyChams session in {launch_mode.value} mode")

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    mission_path = root_dir / 'ros2_ws' / 'src' / 'flychams_bringup' / 'config' / 'mission.yaml'
    
    # Get tmux server
    session_name = "flychams"
    server = libtmux.Server()
    
    # Stop containers in simulation mode
    if launch_mode == LaunchMode.SIMULATION:
        try:
            # Stop all FlyChams containers in parallel
            threads = [
                threading.Thread(target=stop_containers, args=("flychams-COORDINATOR",)),
                threading.Thread(target=stop_containers, args=("flychams-AGENT",)),
                threading.Thread(target=stop_containers, args=("flychams-PX4",)),
                threading.Thread(target=stop_containers, args=("flychams-DASHBOARD",)),
                threading.Thread(target=stop_containers, args=("flychams-SIMULATION",))
            ]

            for t in threads:
                t.start()
            for t in threads:
                t.join()

            # Stop UE5
            subprocess.run("pkill -f 'FlyChamsSim.sh' || true", shell=True)
            
            print("[STOP] FlyChams session stopped successfully")
        except Exception as e:
            print(f"[STOP] Error stopping session: {e}")

    elif launch_mode == LaunchMode.HARDWARE:
        try:
            # Stop coordinator containers on local PC
            stop_containers("flychams-COORDINATOR")
            stop_containers("flychams-DASHBOARD")
            
            # Stop ROS2 processes for each agent
            agents = load_agents(mission_path)
            threads = []
            for agent in agents:
                if agent.ssh.hostname:
                    thread = threading.Thread(target=stop_processes, args=(agent.id, agent.ssh))
                    threads.append(thread)
                else:
                    print(f"[STOP] Warning: Agent {agent.id} missing SSH configuration")

            for t in threads:
                t.start()
            for t in threads:
                t.join()
            
            print("[STOP] FlyChams session stopped successfully")
        except Exception as e:
            print(f"[STOP] Error stopping session: {e}")

    # Wait for 10 seconds
    time.sleep(10)

    # Kill Tmux Session
    try:
        if hasattr(server, "sessions"):
            sessions = server.sessions
        else:
            sessions = server.list_sessions()
            
        for s in sessions:
            name = getattr(s, 'session_name', None) or s.get('session_name')
            if name == session_name:
                s.kill()
                print(f"[STOP] Tmux session '{session_name}' killed.")
                break
        else:
            print(f"[STOP] Tmux session '{session_name}' not found.")
    except Exception as e:
        print(f"[STOP] Error managing tmux session: {e}")

if __name__ == "__main__":
    main()