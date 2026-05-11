#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Building flychams-agent image..."
docker build -t flychams-agent -f "$PROJECT_ROOT/docker/agent.Dockerfile" "$PROJECT_ROOT"
echo "flychams-agent built successfully"