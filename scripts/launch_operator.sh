#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DETACH="${DETACH:-false}"
FOXGLOVE_PORT="${FOXGLOVE_PORT:-8765}"
RECORD="${RECORD:-false}"
RECORD_DIR="${RECORD_DIR:-recordings}"

RECORD_CMD=""
if [ "$RECORD" = "true" ]; then
    RECORD_NAME="flychams_$(date +%Y%m%d_%H%M%S)"
    RECORD_CMD=" & mkdir -p ${RECORD_DIR} && exec ros2 bag record --storage mcap --regex '^(/flychams/.*|/rosout)' --output ${RECORD_DIR}/${RECORD_NAME}"
fi

CMD="source install/setup.bash && ros2 launch flychams_operator operator.launch.py & source install/setup.bash && ros2 launch flychams_operator foxglove.launch.py${RECORD_CMD}" \
    DETACH="$DETACH" FOXGLOVE_PORT="$FOXGLOVE_PORT" "$SCRIPT_DIR/docker/run_operator.sh"