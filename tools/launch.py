#!/usr/bin/env python3
"""
Workflow launcher
"""

from __future__ import annotations

import argparse
import shlex
import subprocess
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

from launchlib.loaders import load_agents  # noqa: E402
from launchlib.types import LaunchMode  # noqa: E402

def tmux(args: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run(["tmux", *args], text=True, capture_output=True)

def tmux_checked(args: list[str]) -> subprocess.CompletedProcess:
    proc = tmux(args)
    if proc.returncode != 0:
        raise RuntimeError(
            "tmux command failed:\n"
            f"  tmux {shlex.join(args)}\n"
            f"  exit_code={proc.returncode}\n"
            f"  stdout={proc.stdout.strip()}\n"
            f"  stderr={proc.stderr.strip()}"
        )
    return proc

def session_exists(session_name: str) -> bool:
    proc = tmux(["has-session", "-t", session_name])
    return proc.returncode == 0

def send_keys(session_name: str, window_name: str, command: str) -> None:
    target = f"{session_name}:{window_name}"
    # Send command + Enter
    tmux_checked(["send-keys", "-t", target, command, "C-m"])

def new_window(session_name: str, window_name: str, cwd: Path) -> None:
    tmux_checked(
        ["new-window", "-t", session_name, "-n", window_name, "-c", str(cwd)]
    )

def setup_kill_key(session_name: str, cmd: str) -> None:
    # Bind Ctrl+K (global to tmux server) to kill THIS session, with confirmation
    prompt = f"Kill tmux session '{session_name}'? (y/n)"
    # Combine run-shell with the command as a single argument for confirm-before
    run_cmd = f"run-shell {shlex.quote(cmd)}"
    tmux_checked(
        [
            "bind-key",
            "-n",
            "C-k",
            "confirm-before",
            "-p",
            prompt,
            run_cmd,
        ]
    )

def setup_simulation(session_name: str, agent_ids: list[str], root_dir: Path) -> None:
    # Get mode flag
    mode_flag = "--sim"

    # Get delays to ensure components are launched in the correct order
    delay_1 = 10.0 # Unreal Engine 5
    delay_2 = 3.0  # Global

    # Create session
    tmux_checked(
        [
            "new-session",
            "-d",
            "-s",
            session_name,
            "-n",
            "UE5",
            "-c",
            str(root_dir),
        ]
    )

    # Bind Ctrl+K to kill this session (with confirmation prompt)
    stop_cmd = f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'stop.py'))} {mode_flag}"
    setup_kill_key(session_name, stop_cmd)

    # Unreal Engine 5
    ue5_cmd = (
        f"bash {shlex.quote(str(root_dir / 'tools' / 'shell' / 'run_ue5.sh'))}"
    )
    send_keys(session_name, "UE5", ue5_cmd)

    # Simulation
    new_window(session_name, "SIMULATION", root_dir)
    simulation_cmd = (
        f"sleep {delay_1} && "
        f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_simulation.py'))} "
    )
    send_keys(session_name, "SIMULATION", simulation_cmd)

    # PX4 SITL
    for idx, _agent_id in enumerate(agent_ids):
        window_name = f"PX4-{idx}"
        new_window(session_name, window_name, root_dir)
        px4_cmd = (
            f"sleep {delay_1} && "
            f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_px4.py'))} "
            f"--agent-index {idx}"
        )
        send_keys(session_name, window_name, px4_cmd)

    # Coordinator
    new_window(session_name, "COORDINATOR", root_dir)
    coordinator_cmd = (
        f"sleep {delay_1} && "
        f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_coordinator.py'))} "
        f"{mode_flag}"
    )
    send_keys(session_name, "COORDINATOR", coordinator_cmd)

    # Agents
    for agent_id in agent_ids:
        window_name = agent_id
        new_window(session_name, window_name, root_dir)
        agent_cmd = (
            f"sleep {delay_1 + delay_2} && "
            f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_agent.py'))} "
            f"--agent-id {shlex.quote(agent_id)} {mode_flag}"
        )
        send_keys(session_name, window_name, agent_cmd)

    # Dashboard
    new_window(session_name, "DASHBOARD", root_dir)
    dashboard_cmd = (
        f"sleep {delay_1} && "
        f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_dashboard.py'))}"
    )
    send_keys(session_name, "DASHBOARD", dashboard_cmd)

    # Focus COORDINATOR
    tmux_checked(["select-window", "-t", f"{session_name}:COORDINATOR"])

    # Attach to session
    subprocess.run(["tmux", "attach", "-t", session_name])

def setup_hardware(session_name: str, agent_ids: list[str], root_dir: Path) -> None:
    # Get mode flag
    mode_flag = "--hardware"

    # Get delays to ensure components are launched in the correct order
    delay_1 = 3.0  # Global

    # Create session
    tmux_checked(
        [
            "new-session",
            "-d",
            "-s",
            session_name,
            "-n",
            "COORDINATOR",
            "-c",
            str(root_dir),
        ]
    )

    # Bind Ctrl+K to kill this session (with confirmation prompt)
    stop_cmd = f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'stop.py'))} {mode_flag}"
    setup_kill_key(session_name, stop_cmd)

    # Coordinator
    coordinator_cmd = (
        f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_coordinator.py'))} "
        f"{mode_flag}"
    )
    send_keys(session_name, "COORDINATOR", coordinator_cmd)

    # Agents
    for agent_id in agent_ids:
        window_name = agent_id
        new_window(session_name, window_name, root_dir)
        agent_cmd = (
            f"sleep {delay_1} && "
            f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_agent.py'))} "
            f"--agent-id {shlex.quote(agent_id)} {mode_flag}"
        )
        send_keys(session_name, window_name, agent_cmd)

    # Dashboard
    new_window(session_name, "DASHBOARD", root_dir)
    dashboard_cmd = (
        f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_dashboard.py'))}"
    )
    send_keys(session_name, "DASHBOARD", dashboard_cmd)

    # Focus COORDINATOR
    tmux_checked(["select-window", "-t", f"{session_name}:COORDINATOR"])

    # Attach to session
    subprocess.run(["tmux", "attach", "-t", session_name])

def main():
    parser = argparse.ArgumentParser(
        description="Launch FlyChams workflow in tmux"
    )
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode")
    args = parser.parse_args()

    # Determine launch mode
    if args.sim and args.hardware:
        print("[LAUNCH] Error: Cannot use both --sim and --hardware flags")
        sys.exit(1)
    elif args.sim:
        launch_mode = LaunchMode.SIMULATION
    elif args.hardware:
        launch_mode = LaunchMode.HARDWARE
    else:
        print("[LAUNCH] Error: No launch mode specified")
        sys.exit(1)

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    mission_path = root_dir / 'ros2_ws' / 'src' / 'flychams_bringup' / 'config' / 'mission.yaml'

    session_name = "flychams"
    if session_exists(session_name):
        print(
            f"[LAUNCH] Error: tmux session already exists: {session_name}\n"
            f"[LAUNCH] Attach with: tmux attach -t {shlex.quote(session_name)}\n"
            f"[LAUNCH] Or kill it with: tmux kill-session -t {shlex.quote(session_name)}",
        )
        sys.exit(1)

    agents = load_agents(mission_path)
    agent_ids = [a.id for a in agents if getattr(a, "id", "")]

    print(f"[LAUNCH] Mode: {launch_mode.value}")
    print(f"[LAUNCH] Mission: {mission_path}")
    print(f"[LAUNCH] Agents ({len(agent_ids)}): {', '.join(agent_ids) if agent_ids else '(none)'}")
    print(f"[LAUNCH] tmux session: {session_name}")

    if launch_mode == LaunchMode.SIMULATION:
        # Setup simulation:
        # 1. UE5
        # 2. PX4 SITL
        # 3. COORDINATOR
        # 4. AGENTS
        # 5. DASHBOARD
        setup_simulation(session_name, agent_ids, root_dir)
    elif launch_mode == LaunchMode.HARDWARE:
        # Setup hardware:
        # 1. COORDINATOR
        # 2. AGENTS
        # 3. DASHBOARD
        setup_hardware(session_name, agent_ids, root_dir)

if __name__ == "__main__":
    main()
