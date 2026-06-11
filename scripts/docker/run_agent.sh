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
    GPU_FLAGS="--runtime nvidia --gpus all -e NVIDIA_DRIVER_CAPABILITIES=all -e NVIDIA_VISIBLE_DEVICES=all"
elif [ "$GPU_VENDOR" = "amd" ]; then
    echo "Using AMD GPU"
    VIDEO_GID=$(getent group video  | cut -d: -f3)
    RENDER_GID=$(getent group render | cut -d: -f3)
    GPU_FLAGS="--device /dev/kfd --device /dev/dri --group-add ${VIDEO_GID} --group-add ${RENDER_GID} --security-opt seccomp=unconfined"
elif [ "$GPU_VENDOR" = "jetson" ]; then
    echo "Using Jetson (Tegra) GPU"
    GPU_FLAGS="--runtime nvidia -e NVIDIA_DRIVER_CAPABILITIES=all"
    if [ -e /dev/nvhost-ctrl ]; then
        GPU_FLAGS="${GPU_FLAGS} --device /dev/nvhost-ctrl"
    fi
    if [ -e /dev/nvhost-ctrl-gpu ]; then
        GPU_FLAGS="${GPU_FLAGS} --device /dev/nvhost-ctrl-gpu"
    fi
    if [ -e /dev/nvhost-vic ]; then
        GPU_FLAGS="${GPU_FLAGS} --device /dev/nvhost-vic"
    fi
    if [ -e /dev/nvhost-nvdec ]; then
        GPU_FLAGS="${GPU_FLAGS} --device /dev/nvhost-nvdec"
    fi
    if [ -e /dev/nvhost-nvenc ]; then
        GPU_FLAGS="${GPU_FLAGS} --device /dev/nvhost-nvenc"
    fi
    if getent group video >/dev/null 2>&1; then
        VIDEO_GID=$(getent group video | cut -d: -f3)
        GPU_FLAGS="${GPU_FLAGS} --group-add ${VIDEO_GID}"
    fi
    if [ -d /usr/lib/aarch64-linux-gnu/tegra ]; then
        GPU_FLAGS="${GPU_FLAGS} -v /usr/lib/aarch64-linux-gnu/tegra:/usr/lib/aarch64-linux-gnu/tegra:ro"
    fi
    shopt -s nullglob
    for nv_plugin in /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstnv*.so; do
        GPU_FLAGS="${GPU_FLAGS} -v ${nv_plugin}:${nv_plugin}:ro"
    done
    shopt -u nullglob
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
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
    -e RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION}" \
    -e CYCLONEDDS_URI="${CYCLONEDDS_URI}" \
    -e GPU_VENDOR="$GPU_VENDOR" \
    -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
    -w "/home/testuser/FlyChams-ROS2" \
    flychams-agent \
    bash -c "source /opt/ros/humble/setup.bash && ${CMD}"