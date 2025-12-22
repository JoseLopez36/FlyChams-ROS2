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
source "$PROJECT_ROOT/.env"

# Default values
AGENT_IMAGE_NAME="flychams-agent"
ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0}

# Set container name
AGENT_CONTAINER_NAME="flychams-${AGENT_ID}"

print_info "Running agent container: $AGENT_CONTAINER_NAME"
print_info "Agent ID: $AGENT_ID"
print_info "ROS Domain ID: $ROS_DOMAIN_ID"

# Function to build the agent ROS2 workspace
build_workspace() 
{
    CMD="source /opt/ros/$ROS_DISTRO/setup.bash && colcon build --symlink-install --packages-up-to flychams_agent"

    # Check if container already exists
    if docker ps -a --format '{{.Names}}' | grep -q "^${AGENT_CONTAINER_NAME}$"; then
        # Check if container is running
        if docker ps --format '{{.Names}}' | grep -q "^${AGENT_CONTAINER_NAME}$"; then
            print_info "Container $AGENT_CONTAINER_NAME is running. Executing commands inside..."
            docker exec -it "$AGENT_CONTAINER_NAME" bash -c "$CMD"
        else
            print_info "Container $AGENT_CONTAINER_NAME exists but is not running. Starting it..."
            docker start "$AGENT_CONTAINER_NAME"
            print_info "Executing commands inside..."
            docker exec -it "$AGENT_CONTAINER_NAME" bash -c "$CMD"
        fi
        return
    fi
    
    # Run the container
    docker run -it --name "$AGENT_CONTAINER_NAME" \
        --network host \
        -e ROS_DOMAIN_ID="$ROS_DOMAIN_ID" \
        -e AGENT_ID="$AGENT_ID" \
        -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
        "$AGENT_IMAGE_NAME" \
        bash -c "$CMD"
}

# Function to run the agent container
run_container() 
{
    if [ -z "$AGENT_ID" ]; then
        print_error "Agent ID is required"
        exit 1
    fi
    
    print_info "Running agent container: $AGENT_CONTAINER_NAME"
    print_info "Agent ID: $AGENT_ID"
    print_info "ROS Domain ID: $ROS_DOMAIN_ID"
    
    # Check if container already exists
    if docker ps -a --format '{{.Names}}' | grep -q "^${AGENT_CONTAINER_NAME}$"; then
        print_warn "Container $AGENT_CONTAINER_NAME already exists. Removing it..."
        docker rm -f "$AGENT_CONTAINER_NAME"
    fi
    
    # Run the container
    docker run -it --name "$AGENT_CONTAINER_NAME" \
        --network host \
        -e ROS_DOMAIN_ID="$ROS_DOMAIN_ID" \
        -e AGENT_ID="$AGENT_ID" \
        -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
        "$AGENT_IMAGE_NAME" \
        bash
}

# Function to stop the agent container
stop_container() 
{
    print_info "Stopping agent container: $AGENT_CONTAINER_NAME"
    if docker ps --format '{{.Names}}' | grep -q "^${AGENT_CONTAINER_NAME}$"; then
        docker stop "$AGENT_CONTAINER_NAME"
        print_info "Container stopped"
    else
        print_warn "Container $AGENT_CONTAINER_NAME is not running"
    fi
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

    # Check if container already exists
    if docker ps -a --format '{{.Names}}' | grep -q "^${AGENT_CONTAINER_NAME}$"; then
        # Check if container is running
        if docker ps --format '{{.Names}}' | grep -q "^${AGENT_CONTAINER_NAME}$"; then
            print_info "Container $AGENT_CONTAINER_NAME is running. Executing commands inside..."
            docker exec -it "$AGENT_CONTAINER_NAME" bash
        else
            print_info "Container $AGENT_CONTAINER_NAME exists but is not running. Starting it..."
            docker start "$AGENT_CONTAINER_NAME"
            print_info "Executing commands inside..."
            docker exec -it "$AGENT_CONTAINER_NAME" bash
        fi
        return
    fi
    
    # Run the container
    docker run -it --name "$AGENT_CONTAINER_NAME" \
        --network host \
        -e ROS_DOMAIN_ID="$ROS_DOMAIN_ID" \
        -e AGENT_ID="$AGENT_ID" \
        -v "$PROJECT_ROOT:/home/testuser/FlyChams-ROS2" \
        "$AGENT_IMAGE_NAME" \
        bash
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

