#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DETACH="${DETACH:-false}"
FOXGLOVE_PORT="${FOXGLOVE_PORT:-8765}"

CMD="source install/setup.bash && ros2 launch flychams_operator operator.launch.py & source install/setup.bash && ros2 launch flychams_operator foxglove.launch.py" \
    DETACH="$DETACH" FOXGLOVE_PORT="$FOXGLOVE_PORT" "$SCRIPT_DIR/docker/run_operator.sh"