#!/usr/bin/env python3
"""
Workflow launcher

Creates a tmux session with one window per instance, launching in order:
  1) GLOBAL
  2) AGENTxx (from config/mission.yaml)
  3) PX4-i (simulation only; i = agent index)
  4) VISUALIZATION
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

def main():
    parser = argparse.ArgumentParser(
        description="Launch FlyChams workflow in tmux (GLOBAL, AGENTs, PX4, VISUALIZATION)"
    )
    parser.add_argument("--sim", action="store_true", help="Run in simulation mode")
    parser.add_argument("--hardware", action="store_true", help="Run in hardware mode")
    parser.add_argument("--delay", type=float, default=1.0, help="Delay in seconds between setup and run")
    args = parser.parse_args()

    # Determine launch mode
    if args.sim and args.hardware:
        print("[GLOBAL] Error: Cannot use both --sim and --hardware flags")
        sys.exit(1)
    elif args.sim:
        launch_mode = LaunchMode.SIMULATION
    elif args.hardware:
        launch_mode = LaunchMode.HARDWARE
    else:
        print("[GLOBAL] Error: No launch mode specified")
        sys.exit(1)

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent
    mission_path = root_dir / 'config' / 'mission.yaml'

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

    # Create session with GLOBAL as first window
    tmux_checked(
        [
            "new-session",
            "-d",
            "-s",
            session_name,
            "-n",
            "GLOBAL",
            "-c",
            str(root_dir),
        ]
    )

    # Bind Ctrl+K to kill this session (with confirmation prompt)
    mode_flag = "--sim" if launch_mode == LaunchMode.SIMULATION else "--hardware"
    stop_cmd = f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'stop.py'))} {mode_flag}"
    setup_kill_key(session_name, stop_cmd)

    # Commands
    global_cmd = (
        f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_global.py'))} "
        f"{mode_flag} --delay {args.delay}"
    )
    send_keys(session_name, "GLOBAL", global_cmd)

    # Agents
    for agent_id in agent_ids:
        window_name = agent_id
        new_window(session_name, window_name, root_dir)
        agent_cmd = (
            f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_agent.py'))} "
            f"--agent-id {shlex.quote(agent_id)} {mode_flag} --delay {args.delay}"
        )
        send_keys(session_name, window_name, agent_cmd)

    # PX4 SITL (simulation only)
    if launch_mode == LaunchMode.SIMULATION:
        for idx, _agent_id in enumerate(agent_ids):
            window_name = f"PX4-{idx}"
            new_window(session_name, window_name, root_dir)
            px4_cmd = (
                f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_px4.py'))} "
                f"--agent-index {idx}"
            )
            send_keys(session_name, window_name, px4_cmd)

    # Visualization (always)
    new_window(session_name, "VISUALIZATION", root_dir)
    viz_cmd = f"python3 -u {shlex.quote(str(root_dir / 'tools' / 'launch_visualization.py'))}"
    send_keys(session_name, "VISUALIZATION", viz_cmd)

    # Focus GLOBAL
    tmux_checked(["select-window", "-t", f"{session_name}:GLOBAL"])

    # Attach to session
    subprocess.run(["tmux", "attach", "-t", session_name])

if __name__ == "__main__":
    main()
