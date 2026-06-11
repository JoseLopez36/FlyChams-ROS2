#!/usr/bin/env bash

CONTAINER_NAME="flychams-micro-xrce-dds"

# Environment variables
CMD="${CMD:-udp4 -p 8888}"
DETACH="${DETACH:-false}"
XRCE_SERIAL_DEVICE="${XRCE_SERIAL_DEVICE:-}"

[ "$DETACH" = "true" ] && RUN_FLAGS="--rm -d" || RUN_FLAGS="--rm -it"

DEVICE_FLAGS=""
if [ -n "$XRCE_SERIAL_DEVICE" ]; then
    if [ -e "$XRCE_SERIAL_DEVICE" ]; then
        DEVICE_FLAGS="--device ${XRCE_SERIAL_DEVICE}"
    else
        echo "WARNING: Serial device ${XRCE_SERIAL_DEVICE} not found on host"
    fi
    if getent group dialout >/dev/null 2>&1; then
        DIALOUT_GID=$(getent group dialout | cut -d: -f3)
        DEVICE_FLAGS="${DEVICE_FLAGS} --group-add ${DIALOUT_GID}"
    fi
fi

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME"
fi

echo "Starting Micro-XRCE-DDS Agent container: $CONTAINER_NAME"
docker run ${RUN_FLAGS} \
    --name "$CONTAINER_NAME" \
    --network host \
    ${DEVICE_FLAGS} \
    micro-xrce-dds-agent \
    $CMD