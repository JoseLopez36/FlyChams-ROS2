#!/bin/bash

# Arguments
RVIZ_CONFIG=${1:-"default.rviz"}

# Launch RViz
ros2 launch flychams_bringup run_rviz.launch.py config:="$RVIZ_CONFIG"