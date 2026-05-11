#!/usr/bin/env bash

containers=(
    $(docker ps --format '{{.Names}}' | grep '^flychams-')
)

if [ ${#containers[@]} -eq 0 ]; then
    echo "No running flychams containers found."
    exit 0
fi

echo "Tailing logs for: ${containers[*]}"
echo "---"

for name in "${containers[@]}"; do
    docker logs -f --tail 50 "$name" 2>&1 | sed "s/^/[${name}] /" &
done

wait