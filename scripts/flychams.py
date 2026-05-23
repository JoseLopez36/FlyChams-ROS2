#!/usr/bin/env python3
import argparse
import subprocess
import os
import time
import yaml

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

def launch_sim(agent_ids: list, record: bool = False, record_dir: str = ""):
    print("=== Simulation mode ===")

    # Operator
    record_env = f"RECORD=true{' RECORD_DIR=' + record_dir if record_dir else ''}" if record else ""
    operator_env = f"DETACH=true {record_env}".strip()
    run(f"{operator_env} {SCRIPT_DIR}/launch_operator.sh")
    time.sleep(1)

    # Micro-XRCE-DDS Agent
    run(f"DETACH=true {SCRIPT_DIR}/launch_micro_xrce_dds.sh")
    time.sleep(1)

    # One PX4 container per agent (instance number taken from mission config idx)
    for agent_id in agent_ids:
        idx = load_agent_idx(agent_id)
        run(f"DETACH=true {SCRIPT_DIR}/launch_px4.sh {idx} {agent_id}")
        time.sleep(1)

    # Coordinator
    run(f"DETACH=true {SCRIPT_DIR}/launch_coordinator.sh")
    time.sleep(1)

    # One agent container per agent
    for agent_id in agent_ids:
        run(f"DETACH=true {SCRIPT_DIR}/launch_agent.sh {agent_id}")
        time.sleep(1)

    # Simulation
    run(f"DETACH=true {SCRIPT_DIR}/launch_simulation.sh")
    time.sleep(1)

# ------------------------------------------------------------------
# Real mode
# ------------------------------------------------------------------

def launch(agent_ids: list):
    print("=== Real mode ===")
    # TODO: implement real mode
    pass

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
        "--record-dir",
        default="",
        metavar="DIR",
        help="Output directory for the bag (default: recordings/ inside the project root)"
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    agent_ids = args.agents if args.agents else load_agent_ids()
    if args.mode == "sim":
        launch_sim(agent_ids, record=args.record, record_dir=args.record_dir)
    else:
        launch(agent_ids)