#!/bin/bash

# Arguments   
IS_SIMULATED=${1:-"False"}

# Launch FlyChams with AirSim
echo "Running FlyingChameleons global..."
ros2 launch flychams_bringup run_global.launch.py is_simulated:=$IS_SIMULATED