#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Building flychams-simulation image..."
docker build -t flychams-simulation -f "$PROJECT_ROOT/docker/simulation.Dockerfile" "$PROJECT_ROOT"
echo "flychams-simulation built successfully"