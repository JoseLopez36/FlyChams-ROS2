#!/bin/bash

# Agent Docker Setup Script
# This script helps set up and run the agent in a Docker container

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

# Get agent ID
AGENT_ID=$2

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Source the environment file
source "$PROJECT_ROOT/setup.sh"

# Default values
AGENT_IMAGE_NAME="flychams-agent"
ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0}
ROS_DISTRO=${ROS_DISTRO:-humble}

# Set container name
AGENT_CONTAINER_NAME="flychams-${AGENT_ID}"

print_info "Running agent container: $AGENT_CONTAINER_NAME"
print_info "Agent ID: $AGENT_ID"
print_info "ROS Domain ID: $ROS_DOMAIN_ID"

# Function to build the agent ROS2 workspace
build_workspace() 
{
    CMD="source /opt/ros/$ROS_DISTRO/setup.bash && cd /home/testuser/FlyChams-ROS2 && colcon build --symlink-install --build-base build/docker --install-base install/docker --packages-up-to flychams_agent"
    "$SCRIPT_DIR/docker/launch_agent.sh" "$AGENT_ID" "$CMD"
}

# Function to run the agent container
run_container() 
{
    CMD="source /opt/ros/$ROS_DISTRO/setup.bash && cd /home/testuser/FlyChams-ROS2 && source install/docker/setup.bash && ros2 launch launch/agent.launch.py agent_id:=$AGENT_ID is_sim:=True"
    "$SCRIPT_DIR/docker/launch_agent.sh" "$AGENT_ID" "$CMD"
}

# Function to stop the agent container
stop_container() 
{
    "$SCRIPT_DIR/docker/stop_agent.sh" "$AGENT_ID"
}

# Function to remove the agent container
remove_container() 
{
    print_info "Removing agent container: $AGENT_CONTAINER_NAME"
    if docker ps -a --format '{{.Names}}' | grep -q "^${AGENT_CONTAINER_NAME}$"; then
        docker rm -f "$AGENT_CONTAINER_NAME"
        print_info "Container removed"
    else
        print_warn "Container $AGENT_CONTAINER_NAME does not exist"
    fi
}

# Function to open a shell in the container
shell_container() 
{
    print_info "Opening shell in agent container"
    "$SCRIPT_DIR/docker/launch_agent.sh" "$AGENT_ID" "bash"
}

# Function to print usage
usage() {
    echo "Usage: $0 {build|run|stop|remove|shell} <agent_id>"
    echo "  build   - Build the agent workspace inside the container"
    echo "  run     - Run the agent launch file inside the container"
    echo "  stop    - Stop the agent container"
    echo "  remove  - Remove the agent container"
    echo "  shell   - Open a bash shell inside the container"
}

case "${1:-help}" in
    build)
        build_workspace
        ;;
    run)
        run_container
        ;;
    stop)
        stop_container
        ;;
    remove)
        remove_container
        ;;
    shell)
        shell_container
        ;;
    help|--help|-h)
        usage
        ;;
    *)
        print_error "Unknown command: $1"
        usage
        exit 1
        ;;
esac

