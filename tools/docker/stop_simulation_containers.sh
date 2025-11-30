#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")/.."

# Stop decentralized containers (docker compose)
if [ -f "$ROOT_DIR/docker/docker-compose.yml" ]; then
    echo "Stopping FlyChams containers..."
    cd "$ROOT_DIR/docker"
    docker compose down
    echo "All FlyChams containers stopped successfully"
fi