#!/bin/bash

# Arguments
IS_SIMULATED=${1:-"False"}
AGENT_ID=${2:-""}

# Check if AGENT_ID is provided
if [ -z "$AGENT_ID" ]; then
  echo "Error: AGENT_ID is required" >&2
  exit 1
fi

# Launch FlyChams with AirSim
echo "Running FlyingChameleons agent..."
ros2 launch flychams_bringup run_agent.launch.py agent_id:=$AGENT_ID is_simulated:=$IS_SIMULATED