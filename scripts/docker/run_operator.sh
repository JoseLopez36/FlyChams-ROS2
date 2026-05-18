#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CONTAINER_NAME="flychams-operator"

# Environment variables
CMD="${CMD:-exec bash}"
DETACH="${DETACH:-false}"
ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}"
RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION:-rmw_cyclonedds_cpp}"
CYCLONEDDS_URI="${CYCLONEDDS_URI:-file:///home/testuser/FlyChams-ROS2/src/flychams_common/config/core/cyclonedds.xml}"
FOXGLOVE_PORT="${FOXGLOVE_PORT:-8765}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting operator container: $CONTAINER_NAME"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --network host \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION}" \
    -e CYCLONEDDS_URI="${CYCLONEDDS_URI}" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    -w "/home/testuser/FlyChams-ROS2" \
    flychams-operator \
    bash -c "source /opt/ros/humble/setup.bash && source install/setup.bash && ${CMD}"