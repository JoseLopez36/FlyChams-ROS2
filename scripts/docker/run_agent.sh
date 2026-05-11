#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

AGENT_ID="${1:-}"
if [ -z "$AGENT_ID" ]; then
    echo "Usage: $0 <AGENT_ID>"
    echo "Example: $0 AGENT00"
    exit 1
fi

CONTAINER_NAME="flychams-agent-${AGENT_ID}"

# Environment variables
CMD="${CMD:-exec bash}"
ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}"
FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS:-UDPv4}"

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting agent container: $CONTAINER_NAME"
docker run --rm -it \
    --name "$CONTAINER_NAME" \
    --privileged \
    --network host \
    --runtime nvidia \
    --gpus all \
    -e NVIDIA_DRIVER_CAPABILITIES=all \
    -e NVIDIA_VISIBLE_DEVICES=all \
    -e AGENT_ID="$AGENT_ID" \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS}" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    -w "/home/testuser/FlyChams-ROS2" \
    flychams-agent \
    bash -c "source /opt/ros/humble/setup.bash && ${CMD}"