#!/usr/bin/env python3
"""
Environment and agent loader classes
"""

from pathlib import Path
import yaml
import os

from .types import Environment, Agent, AgentSSH

def load_environment(path: Path) -> Environment:
    """
    Load environment from .env file
    """
    if path.exists():
        with open(path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                if '=' in line:
                    key, value = line.split('=', 1)
                    value = os.path.expandvars(value)
                    os.environ[key] = value

    else:
        raise FileNotFoundError(f"Config file not found: {path}")
    
    return Environment(
        user_name=os.environ.get('USER', ''),
        docker_user_name=os.environ.get('USER_NAME', ''),
        display=os.environ.get('DISPLAY', ':0'),
        ros_domain_id=os.environ.get('ROS_DOMAIN_ID', '0'),
        flychams_ros2_path=os.environ.get('FLYCHAMS_ROS2_PATH', ''),
        flychams_px4_path=os.environ.get('FLYCHAMS_PX4_PATH', ''),
        flychams_airsim_path=os.environ.get('FLYCHAMS_AIRSIM_PATH', '')
    )

def load_agents(path: Path) -> list[Agent]:
    """
    Load all agents specs from generated config/mission.yaml
    """
    out: list[Agent] = []

    with open(path, "r") as f:
        data = yaml.safe_load(f) or {}

    agents = data.get("/**").get("ros__parameters").get("agents")
    if not isinstance(agents, dict):
        print("mission.yaml agents section is not a mapping")
        return out

    # Get agent IDs and iterate over them
    id_list = agents.get("id_list")
    for id in id_list:
        agent = agents.get(id)
        ssh = AgentSSH(
            hostname=str(agent.get("ssh").get("hostname", "")),
            user=str(agent.get("ssh").get("user", "jetson"))
        )
        out.append(Agent(id=id, ssh=ssh))

    return out

def load_agent(agent_id: str, path: Path) -> Agent:
    """
    Load agent specs from generated mission.yaml
    """
    with open(path, "r") as f:
        data = yaml.safe_load(f) or {}

    agent = data.get("/**").get("ros__parameters").get("agents").get(agent_id)
    if not isinstance(agent, dict):
        print(f"mission.yaml agent {agent_id} section is not a mapping")
        return None

    ssh = AgentSSH(
        hostname=str(agent.get("ssh").get("hostname", "")),
        user=str(agent.get("ssh").get("user", "jetson"))
    )

    return Agent(id=agent_id, ssh=ssh)