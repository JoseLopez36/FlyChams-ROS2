#!/usr/bin/env bash

AGENT_IDX="${1:-}"
if [ -z "$AGENT_IDX" ]; then
    echo "Usage: $0 <AGENT_IDX>"
    echo "Example: $0 0"
    exit 1
fi

CONTAINER_NAME="flychams-px4-${AGENT_IDX}"

# Environment variables
CMD="${CMD:-/bin/bash}"
DETACH="${DETACH:-false}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

# PX4 Docker repository
PX4_DOCKER_REPO="px4io/px4-dev-nuttx-focal:2021-04-29"

# Create cache directory
CCACHE_DIR=${HOME}/.ccache
mkdir -p "${CCACHE_DIR}"

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting PX4 container: $CONTAINER_NAME"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --network host \
    --cpus="1.0" \
    --memory="512m" \
    -e LOCAL_USER_ID="$(id -u)" \
    -e CCACHE_DIR="${CCACHE_DIR}" \
    -v ${CCACHE_DIR}:${CCACHE_DIR}:rw \
    -v ${FLYCHAMS_PX4_PATH}:${FLYCHAMS_PX4_PATH}:rw \
    -w "${FLYCHAMS_PX4_PATH}" \
    ${PX4_DOCKER_REPO} \
    bash -c "$CMD"