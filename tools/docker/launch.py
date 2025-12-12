#!/usr/bin/env python3

import libtmux
import argparse
import time
import os
import yaml
import subprocess
import sys
from pathlib import Path

def get_session(server, session_name):
    # Try property
    if hasattr(server, "sessions"):
        for s in server.sessions:
            if s.session_name == session_name:
                return s
    # Try method
    elif hasattr(server, "list_sessions"):
        for s in server.list_sessions():
            # Handle object vs dict representation
            name = getattr(s, 'session_name', None)
            if name is None and isinstance(s, dict):
                name = s.get('session_name')
            if name == session_name:
                return s
    return None

def get_windows(session):
    if hasattr(session, "windows"):
        return session.windows
    elif hasattr(session, "list_windows"):
        return session.list_windows()
    return []

def get_panes(window):
    if hasattr(window, "panes"):
        return window.panes
    elif hasattr(window, "list_panes"):
        return window.list_panes()
    return []

def parse_agents_yaml(path_str):
    path = Path(path_str)
    with open(path, 'r') as f:
        agents_data = yaml.safe_load(f)

    agents = agents_data['agents']
    
    # Extract agent info
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
    # Get arguments
    parser = argparse.ArgumentParser(description="FlyChams Launch Workflow with Docker")
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode (all instances on same PC)")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode (global on PC, agents on Jetsons)")
    args = parser.parse_args()
    
    # Determine launch mode
    if args.sim and args.hardware:
        print("Error: Cannot use both --sim and --hardware flags")
        sys.exit(1)

    # Default to simulation mode if neither flag is set
    launch_mode = "hardware" if args.hardware else "simulation"
    print(f"Launch mode: {launch_mode}")

    # Parameters
    session_name = "flychams"
    delay = 1.0

    # Get script directory
    script_dir = Path(__file__).resolve().parent
    
    # Get config paths
    root_dir = script_dir.parent.parent
    agents_yaml_path = root_dir / 'config' / 'agents.yaml'

    # Get agent info
    agent_info_list = parse_agents_yaml(agents_yaml_path) 
    num_agents = len(agent_info_list)
    agent_ids = [info['id'] for info in agent_info_list]
    print(f"Found {num_agents} agents: {agent_ids}")

    # Setup tmux session
    server = None
    session = None
    setup_window = None
    
    # Get tmux server
    server = libtmux.Server()
    session = get_session(server, session_name)
    
    if not session:
        # Create new session
        session = server.new_session(session_name=session_name, window_name="Setup")
        print(f"Created new session: {session_name}")
        setup_window = get_windows(session)[0]
        if setup_window.window_name != "Setup":
            setup_window.rename_window("Setup")
    else:
        # Attach to existing session
        print(f"Attached to existing session: {session_name}")
        setup_window = None
        for w in get_windows(session):
            if w.window_name == "Setup":
                setup_window = w
                break
        if not setup_window:
            setup_window = session.new_window(window_name="Setup")

    # --- Setup Stage ---
    # Launch global instance in tmux
    setup_panes = get_panes(setup_window)
    current_pane = setup_panes[0]
    
    launch_global_script = script_dir / "launch_global.py"
    if launch_mode == "hardware":
        global_setup_cmd = f"python3 {launch_global_script} --hardware --mode setup"
    elif launch_mode == "simulation":
        global_setup_cmd = f"python3 {launch_global_script} --sim --mode setup"
    current_pane.send_keys(global_setup_cmd)
        
    # Wait for delay
    time.sleep(delay)
    
    # Launch agent instances in tmux
    for i, agent_info in enumerate(agent_info_list):
        agent_id = agent_info['id']
        print(f"Setting up agent instance for {agent_id}...")
        
        launch_agent_script = script_dir / "launch_agent.py"
        if launch_mode == "hardware":
            agent_setup_cmd = f"python3 {launch_agent_script} --agent-id {agent_id} --agent-index {i} --hardware --mode setup"
        elif launch_mode == "simulation":
            agent_setup_cmd = f"python3 {launch_agent_script} --agent-id {agent_id}  --agent-index {i} --sim --mode setup"
        
        # Launch agent instance in new tmux pane
        current_pane = setup_window.split_window(attach=False)
        setup_window.select_layout('tiled')
        current_pane.send_keys(agent_setup_cmd)

        # Wait for delay
        time.sleep(delay)

    # --- Run Stage ---
    # Get run window for simulation mode
    run_window = None
    for w in get_windows(session):
        if w.window_name == "Run":
            run_window = w
            break
            
    if not run_window:
        run_window = session.new_window(window_name="Run")
        
    run_panes = get_panes(run_window)
    current_pane = run_panes[0]
    
    # Launch global instance in tmux
    launch_global_script = script_dir / "launch_global.py"
    if launch_mode == "hardware":
        global_run_cmd = f"python3 {launch_global_script} --hardware --mode run"
    elif launch_mode == "simulation":
        global_run_cmd = f"python3 {launch_global_script} --sim --mode run"
    current_pane.send_keys(global_run_cmd)
    
    # Launch agent instances in tmux
    for i, agent_id in enumerate(agent_ids):
        print(f"Running agent instance for {agent_id}...")
        
        launch_agent_script = script_dir / "launch_agent.py"
        if launch_mode == "hardware":
            agent_run_cmd = f"python3 {launch_agent_script} --agent-id {agent_id} --agent-index {i} --hardware --mode run"
        elif launch_mode == "simulation":
            agent_run_cmd = f"python3 {launch_agent_script} --agent-id {agent_id} --agent-index {i} --sim --mode run"
        
        # Launch agent instance in new tmux pane
        current_pane = run_window.split_window(attach=False)
        run_window.select_layout('tiled')
        current_pane.send_keys(agent_run_cmd)

    print("Workflow completed successfully")
    print("Press Ctrl+B and d to stop the session...")

    # Attach to session
    print(f"Attaching to session {session_name}...")
    try:
        subprocess.run(["tmux", "attach", "-t", session_name])
    except KeyboardInterrupt:
        pass
    
    print("\nSession detached. Stopping containers...")
    stop_script = script_dir / "stop.py"
    if launch_mode == "hardware":
        subprocess.run([sys.executable, str(stop_script), "--hardware"])
    elif launch_mode == "simulation":
        subprocess.run([sys.executable, str(stop_script), "--sim"])

if __name__ == "__main__":
    main()