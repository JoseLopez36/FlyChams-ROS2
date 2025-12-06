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

def load_env_file(env_file):
    with open(env_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            if '=' in line:
                key, value = line.split('=', 1)
                value = os.path.expandvars(value)
                os.environ[key] = value

def parse_agents_yaml(path_str):
    path = Path(path_str)
    with open(path, 'r') as f:
        agents_data = yaml.safe_load(f)

    agents = agents_data['agents']
    
    # Extract agent IDs
    agent_ids = [agent['id'] for agent in agents if 'id' in agent]
    return agent_ids

def build_docker_run_cmd(name, cmd_inside, env_vars, volumes, image):
    docker_cmd = [
        "docker", "run", "--rm",
        "--name", name,
        "--net=host",
        "--privileged",
    ]
    
    # Add environment variables
    for k, v in env_vars.items():
        docker_cmd.extend(["-e", f"{k}={v}"])
        
    # Add volumes
    for host_path, container_path in volumes.items():
        docker_cmd.extend(["-v", f"{host_path}:{container_path}"])
        
    docker_cmd.append(image)
    docker_cmd.append("bash")
    docker_cmd.append("-c")
    docker_cmd.append(f"'{cmd_inside}'")
    
    # Join into a string for tmux
    return " ".join(docker_cmd)

def build_docker_exec_cmd(container_name, cmd_inside):
    docker_cmd = [
        "docker", "exec", "-it",
    ]
    
    docker_cmd.append(container_name)
    docker_cmd.append("bash")
    docker_cmd.append("-c")
    docker_cmd.append(f"'{cmd_inside}'")
    
    return " ".join(docker_cmd)

def cleanup_container(container_name):
    result = subprocess.run(
        ["docker", "inspect", container_name], 
        stdout=subprocess.DEVNULL, 
        stderr=subprocess.DEVNULL
    )
    if result.returncode == 0:
        print(f"Removing existing container: {container_name}")
        subprocess.run(["docker", "rm", "-f", container_name], stdout=subprocess.DEVNULL)

def main():
    # Get arguments
    parser = argparse.ArgumentParser(description="FlyChams Launch Workflow with Docker")
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    args = parser.parse_args()

    # Get script directory
    script_dir = Path(__file__).resolve().parent
    
    # Get config paths
    root_dir = script_dir.parent.parent
    config_env_path = root_dir / 'docker' / 'config.env'
    agents_yaml_path = root_dir / 'config' / 'agents.yaml'
    
    # Load environment variables
    if config_env_path.exists():
        load_env_file(config_env_path)
    
    user_name = os.environ.get('USER_NAME')
    flychams_ros2_path = os.environ.get('FLYCHAMS_ROS2_PATH')
    flychams_airsim_path = os.environ.get('FLYCHAMS_AIRSIM_PATH')
    flychams_px4_path = os.environ.get('FLYCHAMS_PX4_PATH')
    
    # Get agent IDs
    agent_ids = parse_agents_yaml(agents_yaml_path) 
    num_agents = len(agent_ids)
    print(f"Found {num_agents} agents: {agent_ids}")

    # Get Docker config
    volumes = {
        '/tmp/.X11-unix': '/tmp/.X11-unix',
        flychams_ros2_path: f"/home/{user_name}/FlyChams-ROS2",
        flychams_airsim_path: f"/home/{user_name}/FlyChams-Cosys-AirSim",
        flychams_px4_path: f"/home/{user_name}/PX4-Autopilot"
    }
    
    base_env = {
        'DISPLAY': os.environ.get('DISPLAY', ':0'),
        'QT_X11_NO_MITSHM': '1',
        'ROS_DOMAIN_ID': os.environ.get('ROS_DOMAIN_ID', '0'),
        'FASTDDS_BUILTIN_TRANSPORTS': 'UDPv4',
        'FLYCHAMS_PATH': f"/home/{user_name}/FlyChams-ROS2",
        'AIRSIM_PATH': f"/home/{user_name}/FlyChams-Cosys-AirSim",
        'PX4_PATH': f"/home/{user_name}/PX4-Autopilot",
        'USER_NAME': user_name
    }

    # Parameters
    session_name = "flychams"
    is_simulated = "True" if args.sim else "False"
    delay = 2.0
    long_delay = 3.0
    
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
    # Get setup window
    setup_panes = get_panes(setup_window)
    current_pane = setup_panes[0]
    
    # Launch global container
    print("Setting up global instance...")
    cleanup_container("flychams-global")
    global_setup_cmd = (
        f"source /opt/ros/iron/setup.bash && "
        f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
        f"/home/{user_name}/FlyChams-ROS2/tools/run/setup_global.sh {is_simulated}"
    )
    
    docker_global_cmd = build_docker_run_cmd(
        "flychams-global", 
        global_setup_cmd, 
        base_env, 
        volumes,
        "flychams-ros2:latest"
    )
    current_pane.send_keys(docker_global_cmd)

    # Wait for delay
    time.sleep(delay)
    
    # Launch agent containers
    for i, agent_id in enumerate(agent_ids):
        print(f"Setting up agent instance for {agent_id}...")
        cleanup_container(f"flychams-{agent_id}")
        agent_env = base_env.copy()
        agent_env['AGENT_ID'] = agent_id
        
        # Build command inside container
        px4_cmd_part = ""
        if is_simulated:
            # If simulated, launch PX4 SITL and setup_agent.sh in background
            px4_cmd_part = (
                f"PX4_SIM_HOSTNAME=172.17.0.1 PX4_SIM_MODEL=iris "
                f"/home/{user_name}/PX4-Autopilot/build/px4_sitl_default/bin/px4 "
                f"-i {i} "
                f"-d /home/{user_name}/PX4-Autopilot/ROMFS/px4fmu_common "
                f"-s etc/init.d-posix/rcS"
            )
            
            agent_setup_cmd = (
                f"{px4_cmd_part} & "
                f"sleep {delay} && "
                f"source /opt/ros/iron/setup.bash && "
                f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
                f"/home/{user_name}/FlyChams-ROS2/tools/run/setup_agent.sh {agent_id} {is_simulated}"
            )
        else:
            # If not sim, just run setup_agent.sh
            agent_setup_cmd = (
                f"source /opt/ros/iron/setup.bash && "
                f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
                f"/home/{user_name}/FlyChams-ROS2/tools/run/setup_agent.sh {agent_id} {is_simulated}"
            )

        # Build docker run command
        docker_agent_cmd = build_docker_run_cmd(
            f"flychams-{agent_id}",
            agent_setup_cmd,
            agent_env,
            volumes,
            "flychams-ros2:latest"
        )
        
        current_pane = setup_window.split_window(attach=False)
        setup_window.select_layout('tiled')
        current_pane.send_keys(docker_agent_cmd)

        # Wait for delay
        time.sleep(delay)

    # Wait for long delay between setup and run
    time.sleep(long_delay)

    # --- Run Stage ---
    # Get run window
    run_window = None
    for w in get_windows(session):
        if w.window_name == "Run":
            run_window = w
            break
            
    if not run_window:
        run_window = session.new_window(window_name="Run")
        
    run_panes = get_panes(run_window)
    current_pane = run_panes[0]
    
    # Launch global run
    print("Running global instance...")
    global_run_cmd = (
        f"source /opt/ros/iron/setup.bash && "
        f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
        f"/home/{user_name}/FlyChams-ROS2/tools/run/run_global.sh {is_simulated}"
    )
    
    docker_global_exec = build_docker_exec_cmd(
        "flychams-global",
        global_run_cmd
    )
    current_pane.send_keys(docker_global_exec)
    
    # Launch agent runs
    for agent_id in agent_ids:
        print(f"Running agent instance for {agent_id}...")
        
        agent_run_cmd = (
            f"source /opt/ros/iron/setup.bash && "
            f"source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && "
            f"/home/{user_name}/FlyChams-ROS2/tools/run/run_agent.sh {agent_id} {is_simulated}"
        )
        
        docker_agent_exec = build_docker_exec_cmd(
            f"flychams-{agent_id}",
            agent_run_cmd
        )
        
        current_pane = run_window.split_window(attach=False)
        run_window.select_layout('tiled')
        current_pane.send_keys(docker_agent_exec)

    print("Workflow completed successfully")

    # Attach to session
    print(f"Attaching to session {session_name}...")
    try:
        subprocess.run(["tmux", "attach", "-t", session_name])
    except KeyboardInterrupt:
        pass
    
    print("\nSession detached. Stopping containers...")
    stop_script = script_dir / "stop.py"
    subprocess.run([sys.executable, str(stop_script)])

if __name__ == "__main__":
    main()