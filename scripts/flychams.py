#!/usr/bin/env python3
import argparse
import subprocess
import os
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

    # Start duration timer if specified
    if duration > 0:
        print(f"Mission duration timer set: {duration} seconds")
        timer = threading.Timer(duration, stop_all)
        timer.daemon = True
        timer.start()

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

# ------------------------------------------------------------------
# Real mode
# ------------------------------------------------------------------

def launch(agent_ids: list, duration: float = 0.0):
    print("=== Real mode ===")
    # TODO: implement real mode
    pass

def stop_all():
    """Stop all FlyChams containers."""
    print("\n=== Mission duration expired - stopping all containers ===")
    run(f"{SCRIPT_DIR}/stop.sh")

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