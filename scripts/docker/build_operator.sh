#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Building flychams-operator image..."
docker build -t flychams-operator -f "$PROJECT_ROOT/docker/operator.Dockerfile" "$PROJECT_ROOT"
echo "flychams-operator built successfully"