#!/usr/bin/env python3
"""
Data classes for the launch library
"""

from dataclasses import dataclass
from enum import Enum

class LaunchMode(Enum):
    """Launch mode enumeration"""
    NONE = "NONE"
    SIMULATION = "SIMULATION"
    HARDWARE = "HARDWARE"

@dataclass(frozen=True)
class Environment:
    user_name: str
    docker_user_name: str
    display: str
    ros_domain_id: str
    flychams_ros2_path: str
    flychams_px4_path: str
    flychams_airsim_path: str

@dataclass(frozen=True)
class AgentSSH:
    hostname: str = ""
    user: str = "jetson"

@dataclass(frozen=True)
class Agent:
    id: str = ""
    ssh: AgentSSH = AgentSSH()