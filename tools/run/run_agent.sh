#!/bin/bash
set -e  # Exit on error

# Arguments   
RECORD=${1:-"false"}

# Get ROS2 workspace directory
ROS2_WS="$FLYCHAMS_PATH/ros2_ws"

# Source ROS2 workspace
if [ -f "$ROS2_WS/install/setup.bash" ]; then
  source "$ROS2_WS/install/setup.bash"
else
  echo "Error: ROS workspace not found at $ROS2_WS" >&2
  exit 1
fi

# Launch FlyChams with AirSim
echo "Running FlyingChameleons simulation..."
ros2 launch flychams_bringup run_agent.launch.py