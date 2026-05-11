#!/bin/bash

# Get agent ID
AGENT_ID=$1

if [ -z "$AGENT_ID" ]; then
    echo "Usage: $0 <agent_id>"
    exit 1
fi

CONTAINER_NAME="flychams-${AGENT_ID}"

if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Stopping container ${CONTAINER_NAME}..."
    docker stop "${CONTAINER_NAME}"
else
    echo "Container ${CONTAINER_NAME} is not running."
fi

