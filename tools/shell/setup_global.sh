#!/bin/bash

# Arguments
IS_SIMULATED=${1:-"False"}

# Launch FlyChams with AirSim
echo "Setting up FlyingChameleons global..."
ros2 launch flychams_bringup setup_global.launch.py is_simulated:=$IS_SIMULATED