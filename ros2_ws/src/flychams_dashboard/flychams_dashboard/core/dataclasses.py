"""Data classes for FlyChams Dashboard elements"""

from dataclasses import dataclass
from typing import Optional
from geometry_msgs.msg import Point


@dataclass
class AgentData:
    """Data structure for agent information"""
    position: Optional[Point] = None
    has_position: bool = False
    setpoint: Optional[Point] = None
    has_setpoint: bool = False
    position_sub: Optional[object] = None
    position_setpoint_sub: Optional[object] = None


@dataclass
class TargetData:
    """Data structure for target information"""
    position: Optional[Point] = None
    has_position: bool = False
    position_sub: Optional[object] = None


@dataclass
class ClusterData:
    """Data structure for cluster information"""
    center: Optional[Point] = None
    radius: float = 0.0
    has_geometry: bool = False
    geometry_sub: Optional[object] = None

