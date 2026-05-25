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

def load_agent_ids() -> list:
    with open(MISSION_YAML, "r") as f:
        data = yaml.safe_load(f)
    return data["/**"]["ros__parameters"]["agents"]["id_list"]

def load_agent_idx(agent_id: str) -> int:
    with open(MISSION_YAML, "r") as f:
        data = yaml.safe_load(f)
    return data["/**"]["ros__parameters"]["agents"][agent_id]["idx"]

def run(cmd, **kwargs):
    print(f"+ {cmd}")
    return subprocess.run(cmd, shell=True, **kwargs)

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

    # One PX4 container per agent (instance number taken from mission config idx)
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
# Real mode
# ------------------------------------------------------------------

def launch(agent_ids: list, duration: float = 0.0):
    print("=== Real mode ===")
    # TODO: implement real mode
    pass

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
            print(f"Remaining: {remaining:.0f}s", end="\r", flush=True)
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
            print(f"Elapsed: {elapsed}s", end="\r", flush=True)
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
        choices=["sim", "real"],
        help="Launch mode:\n  sim   Simulation (AirSim + PX4 SITL)\n  real  Real hardware"
    )
    parser.add_argument(
        "agents",
        nargs="*",
        metavar="AGENT_ID",
        help="Agent IDs to launch (e.g. AGENT00 AGENT01). Defaults to all agents in mission.yaml"
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
    if args.mode == "sim":
        launch_sim(agent_ids, record=args.record, record_name=args.record_name, duration=args.duration)
    else:
        launch(agent_ids, duration=args.duration)