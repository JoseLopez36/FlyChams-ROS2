#!/usr/bin/env bash
CONTAINER_NAME="${CONTAINER_NAME:-flychams-operator}"

if ! docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Error: Container '$CONTAINER_NAME' is not running"
    echo "Start it first with: scripts/docker/run_operator.sh"
    exit 1
fi

if [ -n "${CMD:-}" ]; then
    docker exec -it "$CONTAINER_NAME" bash -c "${CMD}"
else
    docker exec -it "$CONTAINER_NAME" bash
fi