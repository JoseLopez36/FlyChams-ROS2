#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")/.."

# Source the environment file relative to the script location
if [ -f "$ROOT_DIR/docker/config.env" ]; then
    source "$ROOT_DIR/docker/config.env"
fi

# Allow X11 forwarding
xhost +local:docker

# Export environment variables
export USER_NAME=$USER_NAME
export FLYCHAMS_ROS2_PATH=$FLYCHAMS_ROS2_PATH
export FLYCHAMS_AIRSIM_PATH=$FLYCHAMS_AIRSIM_PATH
export FLYCHAMS_PX4_PATH=$FLYCHAMS_PX4_PATH
export DISPLAY=$DISPLAY

# Generate compose file
echo "Generating docker-compose.yml..."
python3 "$SCRIPT_DIR/generate_docker_compose.py" --agents-file "$ROOT_DIR/config/agents.yaml" --output "$ROOT_DIR/docker/docker-compose.yml"

# Run docker compose
echo "Starting containers..."
cd "$ROOT_DIR/docker"
docker compose up --remove-orphans