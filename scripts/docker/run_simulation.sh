#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CONTAINER_NAME="flychams-simulation"

# Environment variables
CMD="${CMD:-exec bash}"
DETACH="${DETACH:-false}"
ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}"
FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS:-UDPv4}"
FASTRTPS_DEFAULT_PROFILES_FILE="${FASTRTPS_DEFAULT_PROFILES_FILE:-/home/testuser/FlyChams-ROS2/src/flychams_common/config/core/fastdds.xml}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting simulation container: $CONTAINER_NAME"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --network host \
    --ipc=host \
    -e XDG_RUNTIME_DIR=/tmp \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS}" \
    -e FASTRTPS_DEFAULT_PROFILES_FILE="${FASTRTPS_DEFAULT_PROFILES_FILE}" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    -w "/home/testuser/FlyChams-ROS2" \
    flychams-simulation \
    bash -c "source /opt/ros/humble/setup.bash && ${CMD}"