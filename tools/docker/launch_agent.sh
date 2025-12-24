#! /bin/bash

# Get agent ID
AGENT_ID=$1

if [ -z "$AGENT_ID" ]; then
    echo "Usage: $0 <agent_id>"
    exit 1
fi

# Agent Docker image
AGENT_IMAGE="flychams-agent"

# Container name
CONTAINER_NAME="flychams-${AGENT_ID}"

# Project root (assuming script is in tools/docker)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Source the environment file
if [ -f "$PROJECT_ROOT/setup.sh" ]; then
    source "$PROJECT_ROOT/setup.sh"
fi

ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0}

# Command to run inside the container
CMD=$2

# If no command is provided, run bash
if [ -z "$CMD" ]; then
    if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "Container ${CONTAINER_NAME} is already running. Executing command..."
        docker exec -it "${CONTAINER_NAME}" bash
    elif docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "Container ${CONTAINER_NAME} exists but is stopped. Starting and executing..."
        docker start "${CONTAINER_NAME}"
        docker exec -it "${CONTAINER_NAME}" bash
    else
        echo "Starting new container ${CONTAINER_NAME}..."
        docker run -it --rm --name "${CONTAINER_NAME}" \
            --network host \
            -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
            -e FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS}" \
            -e AGENT_ID="${AGENT_ID}" \
            -v "${PROJECT_ROOT}:/home/testuser/FlyChams-ROS2" \
            "${AGENT_IMAGE}" \
            bash
    fi
else
    if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "Container ${CONTAINER_NAME} is already running. Executing command..."
        docker exec "${CONTAINER_NAME}" bash -c "${CMD}"
    elif docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "Container ${CONTAINER_NAME} exists but is stopped. Starting and executing..."
        docker start "${CONTAINER_NAME}"
        docker exec "${CONTAINER_NAME}" bash -c "${CMD}"
    else
        echo "Starting new container ${CONTAINER_NAME}..."
        docker run --rm --name "${CONTAINER_NAME}" \
            --network host \
            -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID}" \
            -e FASTDDS_BUILTIN_TRANSPORTS="${FASTDDS_BUILTIN_TRANSPORTS}" \
            -e AGENT_ID="${AGENT_ID}" \
            -v "${PROJECT_ROOT}:/home/testuser/FlyChams-ROS2" \
            "${AGENT_IMAGE}" \
            bash -c "${CMD}"
    fi
fi