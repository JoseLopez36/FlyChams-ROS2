#!/usr/bin/env bash

CONTAINER_NAME="flychams-micro-xrce-dds"

# Environment variables
CMD="${CMD:-MicroXRCEAgent udp4 -p 8888}"
DETACH="${DETACH:-false}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting Micro-XRCE-DDS Agent container: $CONTAINER_NAME"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --network host \
    micro-xrce-dds-agent \
    bash -c "$CMD"