#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DETACH="${DETACH:-false}"

CMD="source /home/testuser/FlyChams-Cosys-AirSim/ros2/install/setup.bash && source install/setup.bash && ros2 launch flychams_simulation simulation.launch.py" \
    DETACH="$DETACH" "$SCRIPT_DIR/docker/run_simulation.sh"