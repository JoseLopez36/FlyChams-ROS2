#!/usr/bin/env bash
# Usage: [RECORD_DIR=<path>] scripts/record_operator.sh
# Records all Foxglove-displayed topics (matching ^/flychams/.* and /rosout) into an
# MCAP bag inside the running operator container. Bags land in $PROJECT_ROOT/recordings/
# on the host (volume-mounted at the same path inside the container).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

CONTAINER_NAME="flychams-operator"
RECORD_DIR="${RECORD_DIR:-${PROJECT_ROOT}/recordings}"
RECORD_NAME="flychams_$(date +%Y%m%d_%H%M%S)"

if ! docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Error: Operator container '${CONTAINER_NAME}' is not running." >&2
    echo "Start it first with: scripts/launch_operator.sh" >&2
    exit 1
fi

mkdir -p "$RECORD_DIR"

echo "Recording Foxglove topics → ${RECORD_DIR}/${RECORD_NAME}/"
echo "Press Ctrl+C to stop and finalize the bag."

docker exec -it "$CONTAINER_NAME" bash -c "
    source /opt/ros/humble/setup.bash
    source install/setup.bash 2>/dev/null || true
    mkdir -p /home/testuser/FlyChams-ROS2/recordings
    exec ros2 bag record \
        --storage mcap \
        --regex '^(/flychams/.*|/rosout)' \
        --output /home/testuser/FlyChams-ROS2/recordings/${RECORD_NAME}
"