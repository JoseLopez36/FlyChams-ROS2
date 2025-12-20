#!/bin/bash
# Get the root directory
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"
# Source the environment file
source "$ROOT_DIR/.env"

# Build the Docker image
echo "Building the Docker image '$IMAGE_NAME'..."
if docker build --rm -t $IMAGE_NAME --file $ROOT_DIR/tools/docker/Dockerfile .; then
    echo "Image '$IMAGE_NAME' built successfully."
else
    echo "Failed to build the Docker image."
    exit 1
fi