#!/usr/bin/env bash

stop_container() {
    local name="$1"
    if docker ps --format '{{.Names}}' | grep -q "^${name}$"; then
        echo "Stopping: $name"
        docker stop "$name"
    fi
}

containers=(
    "flychams-coordinator"
    "flychams-simulation"
    "flychams-operator"
    "flychams-micro-xrce-dds"
    $(docker ps --format '{{.Names}}' | grep '^flychams-agent-')
    $(docker ps --format '{{.Names}}' | grep '^flychams-px4-')
)

for name in "${containers[@]}"; do
    stop_container "$name" &
done

wait