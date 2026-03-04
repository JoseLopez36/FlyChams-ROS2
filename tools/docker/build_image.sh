#!/bin/bash
# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

IMAGE_NAME="flychams-agent"

# Check if building for Jetson
if [[ "$1" == "--jetson" ]]; then
    echo "Building for Jetson (Jetpack 6)..."
    DOCKERFILE="Dockerfile.agent.jetson"
else
    echo "Building for standard PC with GPU..."
    DOCKERFILE="Dockerfile.agent.sim"
fi

# Build the Docker image
echo "Building the Docker image '$IMAGE_NAME'..."
if docker build --rm -t $IMAGE_NAME --file $SCRIPT_DIR/$DOCKERFILE .; then
    echo "Image '$IMAGE_NAME' built successfully."
else
    echo "Failed to build the Docker image."
    exit 1
fi