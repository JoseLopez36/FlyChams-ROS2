#!/usr/bin/env python3
import argparse
import subprocess
import os
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

def run(cmd, **kwargs):
    print(f"+ {cmd}")
    return subprocess.run(cmd, shell=True, **kwargs)

# ------------------------------------------------------------------
# Simulation mode
# ------------------------------------------------------------------

def launch_px4(agent_index: int):
    run(f"{SCRIPT_DIR}/launch_px4.sh {agent_index}")

def launch_sim(agent_ids: list):
    print("=== Simulation mode ===")

    # One PX4 container per agent
    for i in range(len(agent_ids)):
        launch_px4(i)

    # Coordinator
    run(f"{SCRIPT_DIR}/launch_coordinator.sh")

    # Simulation
    run(f"{SCRIPT_DIR}/launch_simulation.sh")

    # One agent container per agent
    for agent_id in agent_ids:
        run(f"{SCRIPT_DIR}/launch_agent.sh {agent_id}")

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
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    agent_ids = args.agents if args.agents else load_agent_ids()
    if args.mode == "sim":
        launch_sim(agent_ids)
    else:
        launch(agent_ids)