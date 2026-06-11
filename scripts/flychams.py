#!/usr/bin/env python3
import argparse
import subprocess
import os
import sys
import time
import yaml
import threading

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))

MISSION_YAML = os.path.join(
    PROJECT_ROOT,
    "src", "flychams_common", "config", "generated", "mission.yaml"
)

HARDWARE_YAML = os.path.join(
    PROJECT_ROOT,
    "src", "flychams_common", "config", "core", "hardware.yaml"
)

def load_agent_ids() -> list:
    with open(MISSION_YAML, "r") as f:
        data = yaml.safe_load(f)
    return data["/**"]["ros__parameters"]["agents"]["id_list"]

def load_agent_idx(agent_id: str) -> int:
    with open(MISSION_YAML, "r") as f:
        data = yaml.safe_load(f)
    return data["/**"]["ros__parameters"]["agents"][agent_id]["idx"]

def load_hardware_config() -> dict:
    """Load hardware agent SSH configuration."""
    if not os.path.exists(HARDWARE_YAML):
        return {}
    with open(HARDWARE_YAML, "r") as f:
        data = yaml.safe_load(f)
    return data.get("agents", {})

def run(cmd, **kwargs):
    print(f"+ {cmd}")
    return subprocess.run(cmd, shell=True, **kwargs)

def run_ssh(host: str, user: str, remote_cmd: str, **kwargs):
    """Run a command on a remote host via SSH."""
    ssh_cmd = f"ssh -o ConnectTimeout=10 -o StrictHostKeyChecking=no {user}@{host} '{remote_cmd}'"
    print(f"+ [SSH {host}] {remote_cmd}")
    return subprocess.run(ssh_cmd, shell=True, **kwargs)

# ------------------------------------------------------------------
# Simulation mode
# ------------------------------------------------------------------

def launch_sim(agent_ids: list, record: bool = False, record_name: str = "", duration: float = 0.0):
    print("=== Simulation mode ===")

    delay = 0.5

    # Operator
    if record:
        if record_name:
            record_dir = f"recordings/"
            record_env = f"RECORD=true RECORD_DIR={record_dir} RECORD_NAME={record_name}"
        else:
            record_env = "RECORD=true"
    else:
        record_env = ""
    operator_env = f"DETACH=true {record_env}".strip()
    run(f"{operator_env} {SCRIPT_DIR}/launch_operator.sh")
    time.sleep(delay)

    # Micro-XRCE-DDS Agent
    run(f"DETACH=true {SCRIPT_DIR}/launch_micro_xrce_dds.sh")
    time.sleep(delay)

    # One PX4 container per agent
    for agent_id in agent_ids:
        idx = load_agent_idx(agent_id)
        run(f"DETACH=true {SCRIPT_DIR}/launch_px4.sh {idx} {agent_id}")
        time.sleep(delay)

    # Coordinator
    run(f"DETACH=true {SCRIPT_DIR}/launch_coordinator.sh")
    time.sleep(delay)

    # One agent container per agent
    for agent_id in agent_ids:
        run(f"DETACH=true {SCRIPT_DIR}/launch_agent.sh {agent_id}")
        time.sleep(delay)

    # Simulation
    run(f"DETACH=true {SCRIPT_DIR}/launch_simulation.sh")
    time.sleep(delay)

    # Mission timer with keyboard early stop
    stop_event = threading.Event()
    listener = threading.Thread(target=keyboard_listener, args=(stop_event,))
    listener.daemon = True
    listener.start()
    wait_for_stop(stop_event, duration)
    stop_all()

# ------------------------------------------------------------------
# Hardware mode
# ------------------------------------------------------------------

def launch_hardware(agent_ids: list, record: bool = False, record_name: str = "", duration: float = 0.0):
    print("=== Hardware mode ===")

    delay = 0.5

    # Load hardware.yaml
    hardware_config = load_hardware_config()
    if not hardware_config:
        print("WARNING: No hardware configuration found.")
        print(f"Please configure agent hosts in: {HARDWARE_YAML}")
        print("Agents will not be launched automatically.")

    # Operator
    if record:
        if record_name:
            record_dir = "recordings/"
            record_env = f"RECORD=true RECORD_DIR={record_dir} RECORD_NAME={record_name}"
        else:
            record_env = "RECORD=true"
    else:
        record_env = ""
    operator_env = f"DETACH=true {record_env}".strip()
    run(f"{operator_env} {SCRIPT_DIR}/launch_operator.sh")
    time.sleep(delay)

    # Coordinator
    run(f"DETACH=true {SCRIPT_DIR}/launch_coordinator.sh")
    time.sleep(delay)

    # Launch agents and Micro-XRCE-DDS on UAVs via SSH
    print(f"Launching {len(agent_ids)} agents on UAVs...")
    for agent_id in agent_ids:
        if agent_id in hardware_config:
            cfg = hardware_config[agent_id]
            host = cfg.get("host")
            user = cfg.get("user")
            workspace = cfg.get("workspace")
            idx = load_agent_idx(agent_id)
            xrce_port = 8888 + idx
            serial_device = cfg.get("serial_device", "")

            if host:
                # Launch Micro-XRCE-DDS Agent
                if serial_device:
                    remote_xrce_cmd = (
                        f"cd {workspace} && DETACH=true "
                        f"XRCE_SERIAL_DEVICE={serial_device} scripts/launch_micro_xrce_dds.sh"
                    )
                    print(f"Launching Micro-XRCE-DDS for {agent_id} on {host} ({serial_device})...")
                else:
                    remote_xrce_cmd = (
                        f"cd {workspace} && DETACH=true "
                        f"XRCE_PORT={xrce_port} scripts/launch_micro_xrce_dds.sh"
                    )
                    print(f"Launching Micro-XRCE-DDS for {agent_id} on {host}:{xrce_port}...")
                run_ssh(host, user, remote_xrce_cmd)
                time.sleep(0.5)

                # Launch flychams_agent
                remote_cmd = f"cd {workspace} && DETACH=true scripts/launch_agent.sh {agent_id}"
                print(f"Launching {agent_id} on {host}...")
                run_ssh(host, user, remote_cmd)
            else:
                print(f"WARNING: No host configured for {agent_id}")
        else:
            print(f"WARNING: {agent_id} not found in hardware.yaml, skipping")
        time.sleep(delay)

    # Mission timer with keyboard early stop
    stop_event = threading.Event()
    listener = threading.Thread(target=keyboard_listener, args=(stop_event,))
    listener.daemon = True
    listener.start()
    wait_for_stop(stop_event, duration)
    stop_all()

    # Stop remote agents and XRCE via SSH
    print("Stopping remote agents and Micro-XRCE-DDS...")
    for agent_id in agent_ids:
        if agent_id in hardware_config:
            cfg = hardware_config[agent_id]
            host = cfg.get("host")
            user = cfg.get("user")
            if host:
                remote_cmd = f"cd {workspace} && scripts/stop.sh 2>/dev/null || true"
                run_ssh(host, user, remote_cmd, capture_output=True)

def stop_all():
    """Stop all FlyChams containers."""
    print("\n=== Stopping all containers ===")
    run(f"{SCRIPT_DIR}/stop.sh")

def keyboard_listener(stop_event):
    """Listen for Enter key to trigger early stop."""
    try:
        input()
        stop_event.set()
    except EOFError:
        pass

def wait_for_stop(stop_event, duration=0):
    """Wait for duration or early stop. Returns True if stopped early."""
    if duration > 0:
        # Countdown mode
        print(f"Mission running for {duration} seconds (press Enter to stop early)...")
        remaining = duration
        while remaining > 0 and not stop_event.is_set():
            print(f"Remaining: {remaining:.0f}s".ljust(30), end="\r", flush=True)
            sleep_time = min(1.0, remaining)
            stop_event.wait(sleep_time)
            remaining -= sleep_time
        if stop_event.is_set():
            print("\nStopped early by user")
            return True
        print("Remaining: 0s")
        return False
    else:
        # Up counter mode
        print("Mission running (press Enter to stop)...")
        elapsed = 0
        while not stop_event.is_set():
            print(f"Elapsed: {elapsed}s".ljust(30), end="\r", flush=True)
            stop_event.wait(1.0)
            elapsed += 1
        print(f"\nStopped after {elapsed}s")
        return True

# ------------------------------------------------------------------
# Entry point
# ------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="FlyChams launcher",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "mode",
        choices=["sim", "hw"],
        help="Launch mode:\n  sim   Simulation (AirSim + PX4 SITL + all containers)\n  hw  Hardware mode (GCS bringup: operator, coordinator, Micro-XRCE-DDS Agent)"
    )
    parser.add_argument(
        "agents",
        nargs="*",
        metavar="AGENT_ID",
        help="Agent IDs to launch (sim mode only, e.g. AGENT00 AGENT01). Defaults to all agents in mission.yaml"
    )
    parser.add_argument(
        "--record",
        action="store_true",
        default=False,
        help="Record all Foxglove-displayed topics to an MCAP bag (operator only)"
    )
    parser.add_argument(
        "--record-name",
        default="",
        metavar="NAME",
        help="Custom name for the recording (creates recordings/NAME/NAME.mcap, default: auto-generated timestamp)"
    )
    parser.add_argument(
        "--duration",
        type=float,
        default=0.0,
        metavar="SECONDS",
        help="Mission duration in seconds (0 = run indefinitely, stop everything when expired)"
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    agent_ids = args.agents if args.agents else load_agent_ids()
    if args.mode == "hw":
        # Hardware mode
        launch_hardware(agent_ids, record=args.record, record_name=args.record_name, duration=args.duration)
    else:
        # Simulation mode
        launch_sim(agent_ids, record=args.record, record_name=args.record_name, duration=args.duration)