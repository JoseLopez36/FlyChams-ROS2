#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DETACH="${DETACH:-false}"

CMD="source install/setup.bash && ros2 launch flychams_coordinator coordinator.launch.py" \
    DETACH="$DETACH" "$SCRIPT_DIR/docker/run_coordinator.sh"