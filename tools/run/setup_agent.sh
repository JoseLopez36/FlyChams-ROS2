#!/bin/bash

# Arguments
AGENT_ID=${1:-""}
IS_SIMULATED=${2:-"False"}

# Check if AGENT_ID is provided
if [ -z "$AGENT_ID" ]; then
  echo "Error: AGENT_ID is required" >&2
  exit 1
fi

# Launch FlyChams with AirSim
echo "Setting up FlyingChameleons agent..."
ros2 launch flychams_bringup setup_agent.launch.py agent_id:=$AGENT_ID is_simulated:=$IS_SIMULATED