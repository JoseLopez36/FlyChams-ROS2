#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CONTAINER_NAME="flychams-simulation"

# Environment variables
CMD="${CMD:-exec bash}"
DETACH="${DETACH:-false}"
ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}"
FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS:-UDPv4}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting simulation container: $CONTAINER_NAME"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --privileged \
    --network host \
    --runtime nvidia \
    --gpus all \
    -e NVIDIA_DRIVER_CAPABILITIES=all \
    -e NVIDIA_VISIBLE_DEVICES=all \
    -e XDG_RUNTIME_DIR=/tmp \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS}" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    -w "/home/testuser/FlyChams-ROS2" \
    flychams-simulation \
    bash -c "source /opt/ros/humble/setup.bash && ${CMD}"