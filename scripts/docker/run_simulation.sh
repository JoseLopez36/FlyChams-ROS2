#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CONTAINER_NAME="flychams-simulation"

# Environment variables
CMD="${CMD:-exec bash}"
DETACH="${DETACH:-false}"
ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}"
RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION:-rmw_cyclonedds_cpp}"
CYCLONEDDS_URI="${CYCLONEDDS_URI:-file:///home/testuser/FlyChams-ROS2/src/flychams_common/config/core/cyclonedds.xml}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

# Auto-detect GPU vendor if not specified
GPU_VENDOR="${GPU_VENDOR:-auto}"
if [ "$GPU_VENDOR" = "auto" ]; then
    GPU_VENDOR=$($SCRIPT_DIR/detect_gpu.sh)
fi

# Build GPU-specific Docker flags
GPU_FLAGS=""
if [ "$GPU_VENDOR" = "nvidia" ]; then
    echo "Using NVIDIA GPU"
    VIDEO_GID=$(getent group video  | cut -d: -f3)
    GPU_FLAGS="--runtime nvidia --gpus all --device /dev/dri --group-add ${VIDEO_GID} -e NVIDIA_DRIVER_CAPABILITIES=all -e NVIDIA_VISIBLE_DEVICES=all"
elif [ "$GPU_VENDOR" = "amd" ]; then
    echo "Using AMD GPU"
    VIDEO_GID=$(getent group video  | cut -d: -f3)
    RENDER_GID=$(getent group render | cut -d: -f3)
    GPU_FLAGS="--device /dev/kfd --device /dev/dri --group-add ${VIDEO_GID} --group-add ${RENDER_GID} --security-opt seccomp=unconfined"
fi

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting simulation container: $CONTAINER_NAME (GPU_VENDOR=$GPU_VENDOR)"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --privileged \
    --network host \
    ${GPU_FLAGS} \
    -e XDG_RUNTIME_DIR=/tmp \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION}" \
    -e CYCLONEDDS_URI="${CYCLONEDDS_URI}" \
    -e HW_VENDOR="$GPU_VENDOR" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    -w "/home/testuser/FlyChams-ROS2" \
    flychams-simulation \
    bash -c "source /opt/ros/humble/setup.bash && ${CMD}"