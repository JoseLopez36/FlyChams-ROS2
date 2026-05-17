#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CMD="source /opt/ros/humble/setup.bash && colcon build --packages-select flychams_api flychams_common flychams_coordinator" \
    "$SCRIPT_DIR/docker/run_coordinator.sh"