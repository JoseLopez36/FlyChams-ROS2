#!/bin/bash

AGENT_COUNT=${1:-3}

if ! [[ "$AGENT_COUNT" =~ ^[0-9]+$ ]]; then
    echo "Invalid agent count: ${AGENT_COUNT}"
    exit 1
fi

pkill -9 -f FlyChamsSim >/dev/null 2>&1 || true

docker ps -a --format '{{.Names}}' | grep -E '^PX4-[0-9]+$' | xargs -r docker rm -f >/dev/null 2>&1 || true

TCP_PORTS=()
UDP_PORTS=()

for ((i = 0; i < AGENT_COUNT; i++)); do
    TCP_PORTS+=("$((4560 + i))/tcp")
    UDP_PORTS+=("$((14540 + i))/udp")
    UDP_PORTS+=("$((14556 + i))/udp")
    UDP_PORTS+=("$((14580 + i))/udp")
done

if [ ${#TCP_PORTS[@]} -gt 0 ]; then
    fuser -k "${TCP_PORTS[@]}" >/dev/null 2>&1 || true
fi

if [ ${#UDP_PORTS[@]} -gt 0 ]; then
    fuser -k "${UDP_PORTS[@]}" >/dev/null 2>&1 || true
fi