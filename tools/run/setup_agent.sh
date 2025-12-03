#!/bin/bash
set -e  # Exit on error

# Arguments
AGENT_ID=${1:-""}
IS_SIMULATED=${2:-"False"}

# Check if AGENT_ID is provided
if [ -z "$AGENT_ID" ]; then
  echo "Error: AGENT_ID is required" >&2
  exit 1
fi

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
echo "Setting up FlyingChameleons agent..."
ros2 launch flychams_bringup setup_agent.launch.py agent_id:=$AGENT_ID is_simulated:=$IS_SIMULATED