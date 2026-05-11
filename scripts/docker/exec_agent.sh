#!/usr/bin/env bash
AGENT_ID="${1:-}"
if [ -z "$AGENT_ID" ]; then
    echo "Usage: $0 <AGENT_ID>"
    echo "Example: $0 AGENT00"
    exit 1
fi

CONTAINER_NAME="flychams-agent-${AGENT_ID}"

if ! docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Error: Container '$CONTAINER_NAME' is not running"
    echo "Start it first with: scripts/run_agent.sh $AGENT_ID"
    exit 1
fi

if [ -n "${CMD:-}" ]; then
    docker exec -it "$CONTAINER_NAME" bash -c "${CMD}"
else
    docker exec -it "$CONTAINER_NAME" bash
fi