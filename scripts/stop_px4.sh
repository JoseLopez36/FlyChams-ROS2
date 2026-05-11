#!/bin/bash

# Get agent index
AGENT_INDEX=$1

if [ -z "$AGENT_INDEX" ]; then
    echo "Usage: $0 <agent_index>"
    exit 1
fi

CONTAINER_NAME="PX4-${AGENT_INDEX}"

if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Stopping container ${CONTAINER_NAME}..."
    docker stop "${CONTAINER_NAME}"
else
    echo "Container ${CONTAINER_NAME} is not running."
fi

