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
DETACH="${DETACH:-false}"
ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}"
FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS:-UDPv4}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

# Auto-detect GPU vendor if not specified
GPU_VENDOR="${GPU_VENDOR:-auto}"
if [ "$GPU_VENDOR" = "auto" ]; then
    GPU_VENDOR=$($SCRIPT_DIR/detect_gpu.sh)
    echo "Auto-detected GPU vendor: $GPU_VENDOR"
fi

# Build GPU-specific Docker flags
GPU_FLAGS=""
if [ "$GPU_VENDOR" = "nvidia" ]; then
    echo "Using NVIDIA GPU runtime"
    GPU_FLAGS="--runtime nvidia --gpus all -e NVIDIA_DRIVER_CAPABILITIES=all -e NVIDIA_VISIBLE_DEVICES=all"
elif [ "$GPU_VENDOR" = "amd" ]; then
    echo "Using AMD GPU (ROCm/VAAPI)"
    GPU_FLAGS="--device /dev/kfd --device /dev/dri --group-add video --group-add render -e ROCR_VISIBLE_DEVICES=all"
elif [ "$GPU_VENDOR" = "intel" ]; then
    echo "Using Intel GPU (VAAPI)"
    GPU_FLAGS="--device /dev/dri --group-add video -e LIBVA_DRIVER_NAME=iHD"
else
    echo "No GPU detected or GPU_VENDOR=none - running without GPU acceleration"
fi

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting agent container: $CONTAINER_NAME"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --privileged \
    --network host \
    ${GPU_FLAGS} \
    -e XDG_RUNTIME_DIR=/tmp \
    -e AGENT_ID="$AGENT_ID" \
    -e AGENT_IDX="${AGENT_IDX:-0}" \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS}" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    -w "/home/testuser/FlyChams-ROS2" \
    flychams-agent \
    bash -c "source /opt/ros/humble/setup.bash && ${CMD}"