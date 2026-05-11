#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

CONTAINER_NAME="flychams-simulation"
CMD="${CMD:-exec bash}"

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting simulation container: $CONTAINER_NAME"
docker run --rm -it \
    --name "$CONTAINER_NAME" \
    --privileged \
    --network host \
    --runtime nvidia \
    --gpus all \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS}" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    flychams-simulation \
    bash -c "source /opt/ros/humble/setup.bash && ${CMD}"