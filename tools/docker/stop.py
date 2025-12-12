#!/usr/bin/env python3

import libtmux
import subprocess
import threading
import argparse
import yaml
import os
import sys
from pathlib import Path

def build_ssh_cmd(remote_host, cmd):
    """Build SSH command to execute on remote host"""
    user = remote_host.get('user', 'jetson')
    hostname = remote_host['hostname']
    ssh_key = remote_host.get('ssh_key', '')
    ssh_key_opt = f"-i {ssh_key}" if ssh_key else ""
    return f"ssh {ssh_key_opt} {user}@{hostname} '{cmd}'"

def stop_containers(filter_name):
    subprocess.run(f"docker ps -q --filter name={filter_name} | xargs -r docker stop", shell=True)

def stop_processes(agent_id, remote_host):
    """Stop ROS2 launch processes for an agent on remote Jetson"""
    cmd = (
        f"pkill -f 'setup_agent.launch.py.*agent_id:={agent_id}' || true; "
        f"pkill -f 'run_agent.launch.py.*agent_id:={agent_id}' || true"
    )
    ssh_cmd = build_ssh_cmd(remote_host, cmd)
    subprocess.run(ssh_cmd, shell=True)
    print(f"Stopped ROS2 processes for agent {agent_id} on {remote_host['hostname']}")

def parse_agents_yaml(path_str):
    """Parse agents.yaml to get agent info including SSH config"""
    path = Path(path_str)
    with open(path, 'r') as f:
        agents_data = yaml.safe_load(f)

    agents = agents_data['agents']
    
    # Extract agent info (ID and optional SSH connection info)
    agent_info = []
    for agent in agents:
        if 'id' in agent:
            info = {'id': agent['id']}
            # Optional SSH connection info for hardware mode
            if 'ssh' in agent:
                info['ssh'] = agent['ssh']
            agent_info.append(info)
    
    return agent_info

def main():
    parser = argparse.ArgumentParser(description="Stop FlyChams containers")
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode")
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
    agents_yaml_path = root_dir / 'config' / 'agents.yaml'
    
    # Get tmux server
    session_name = "flychams"
    server = libtmux.Server()
    
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
                print(f"Tmux session '{session_name}' killed.")
                break
        else:
            print(f"Tmux session '{session_name}' not found.")
    except Exception as e:
        print(f"Error managing tmux session: {e}")
    
    # Stop containers in simulation mode
    if launch_mode == "simulation":
        try:
            # Stop all FlyChams containers in parallel
            threads = [
                threading.Thread(target=stop_containers, args=("flychams-global",)),
                threading.Thread(target=stop_containers, args=("flychams-AGENT*",)),
            ]

            for t in threads:
                t.start()
            for t in threads:
                t.join()
            
            print("FlyChams session stopped successfully")
        except Exception as e:
            print(f"Error stopping session: {e}")

    # Stop global container (local) and ROS2 processes (in Jetson) in hardware mode
    elif launch_mode == "hardware":
        try:
            # Stop global container (local PC)
            stop_containers("flychams-global")
            
            # Stop ROS2 processes for each agent
            if agents_yaml_path.exists():
                agent_info_list = parse_agents_yaml(agents_yaml_path)
                threads = []
                
                for agent_info in agent_info_list:
                    agent_id = agent_info['id']
                    remote_host = agent_info.get('ssh')
                    
                    if remote_host:
                        # Stop remote ROS2 processes
                        thread = threading.Thread(
                            target=stop_processes,
                            args=(agent_id, remote_host)
                        )
                        threads.append(thread)
                    else:
                        print(f"Warning: Agent {agent_id} missing SSH configuration")
                
                for t in threads:
                    t.start()
                for t in threads:
                    t.join()
            else:
                print("Warning: agents.yaml not found, cannot stop remote processes")
            
            print("FlyChams session stopped successfully")
        except Exception as e:
            print(f"Error stopping session: {e}")

if __name__ == "__main__":
    main()