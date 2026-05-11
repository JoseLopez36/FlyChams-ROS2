#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DETACH="${DETACH:-false}"
FOXGLOVE_PORT="${FOXGLOVE_PORT:-8765}"

CMD="ros2 launch foxglove_bridge foxglove_bridge_launch.xml port:=${FOXGLOVE_PORT}" \
    DETACH="$DETACH" FOXGLOVE_PORT="$FOXGLOVE_PORT" "$SCRIPT_DIR/docker/run_operator.sh"