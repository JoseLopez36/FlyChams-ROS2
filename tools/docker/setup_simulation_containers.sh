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

# Default agents
AGENTS=1

# Parse args
while [[ $# -gt 0 ]]; do
  case $1 in
    --agents)
      AGENTS="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

# Export environment variables
export USER_NAME=$USER_NAME
export FLYCHAMS_ROS2_PATH=$FLYCHAMS_ROS2_PATH
export FLYCHAMS_AIRSIM_PATH=$FLYCHAMS_AIRSIM_PATH
export FLYCHAMS_PX4_PATH=$FLYCHAMS_PX4_PATH
export DISPLAY=$DISPLAY

# Generate compose file
echo "Generating docker-compose.yml for $AGENTS agents..."
python3 "$SCRIPT_DIR/generate_docker_compose.py" --agents $AGENTS --output "$ROOT_DIR/docker/docker-compose.yml"

# Run docker compose
echo "Starting containers..."
cd "$ROOT_DIR/docker"
docker compose up --remove-orphans