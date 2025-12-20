#!/bin/bash

# Get ROS2 workspace directory
ROS_WS="$FLYCHAMS_PATH/ros2_ws"

# Source ROS2 workspace
if [ -f "$ROS_WS/install/setup.bash" ]; then
  source "$ROS_WS/install/setup.bash"
else
  echo "Error: ROS workspace not found at $ROS_WS" >&2
  exit 1
fi

# Launch FlyChams with AirSim
echo "Running FlyingChameleons simulation..."
ros2 launch flychams_bringup run_simulation.launch.py