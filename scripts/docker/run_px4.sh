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
    --privileged \
    --network host \
    -e AWS_ACCESS_KEY_ID \
    -e AWS_SECRET_ACCESS_KEY \
    -e BRANCH_NAME \
    -e CCACHE_DIR="${CCACHE_DIR}" \
    -e CI \
    -e CODECOV_TOKEN \
    -e COVERALLS_REPO_TOKEN \
    -e LOCAL_USER_ID="$(id -u)" \
    -e PX4_ASAN \
    -e PX4_MSAN \
    -e PX4_TSAN \
    -e PX4_UBSAN \
    -e TRAVIS_BRANCH \
    -e TRAVIS_BUILD_ID \
    -v ${CCACHE_DIR}:${CCACHE_DIR}:rw \
    -v ${FLYCHAMS_PX4_PATH}:${FLYCHAMS_PX4_PATH}:rw \
    -w "${FLYCHAMS_PX4_PATH}" \
    ${PX4_DOCKER_REPO} \
    bash -c "$CMD"