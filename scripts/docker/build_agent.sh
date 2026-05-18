#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Auto-detect GPU vendor if not specified
GPU_VENDOR="${GPU_VENDOR:-auto}"
if [ "$GPU_VENDOR" = "auto" ]; then
    GPU_VENDOR=$($SCRIPT_DIR/detect_gpu.sh)
fi

echo "Building flychams-agent image (GPU_VENDOR=$GPU_VENDOR)..."
docker build \
    --build-arg GPU_VENDOR="$GPU_VENDOR" \
    -t flychams-agent \
    -f "$PROJECT_ROOT/docker/agent.Dockerfile" \
    "$PROJECT_ROOT"
echo "flychams-agent built successfully"