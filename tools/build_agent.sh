#!/bin/bash

# Agent Docker Build Script
# This script helps build the agent in a Docker container

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions to print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Source the environment file
source "$PROJECT_ROOT/setup.sh"

# Default values
AGENT_IMAGE_NAME="flychams-agent"
ROS_DISTRO=${ROS_DISTRO:-humble}

# Set container name
AGENT_CONTAINER_NAME="flychams-agent-build"

print_info "Building agent ROS2 workspace..."

# Build the agent ROS2 workspace
CMD="source /opt/ros/$ROS_DISTRO/setup.bash && cd /home/testuser/FlyChams-ROS2 && colcon build --symlink-install --build-base build/docker --install-base install/docker --packages-up-to flychams_agent"
"$SCRIPT_DIR/docker/launch_agent.sh" "0" "$CMD"