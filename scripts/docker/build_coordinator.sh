#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Building flychams-coordinator image..."
docker build -t flychams-coordinator -f "$PROJECT_ROOT/docker/coordinator.Dockerfile" "$PROJECT_ROOT"
echo "flychams-coordinator built successfully"