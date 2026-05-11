#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Building flychams-base image..."
docker build -t flychams-base -f "$PROJECT_ROOT/docker/base.Dockerfile" "$PROJECT_ROOT"
echo "flychams-base built successfully"