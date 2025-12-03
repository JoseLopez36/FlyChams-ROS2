#!/bin/bash

# Array to store background job PIDs
declare -a bg_pids=()

# Cleanup function to kill all background processes
cleanup() {
    echo ""
    echo "Stopping all processes..."
    for pid in "${bg_pids[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    wait
    exit 0
}

# Set up signal handler for Ctrl+C
trap cleanup SIGINT SIGTERM

# Function to run command in container as a separate process
run_in_container() {
    local container=$1
    local script=$2
    
    echo "Starting simulation in $container..."
    
    # Get FLYCHAMS_PATH from container environment
    FLYCHAMS_PATH=$(docker exec "$container" printenv FLYCHAMS_PATH)
    if [ -z "$FLYCHAMS_PATH" ]; then
        echo "Error: FLYCHAMS_PATH not set in $container. Using default." >&2
        FLYCHAMS_PATH="/home/testuser/FlyChams-ROS2"
    fi
    
    # Trim whitespace
    FLYCHAMS_PATH=$(echo "$FLYCHAMS_PATH" | tr -d '\r')

    # Get AGENT_ID if available
    AGENT_ID_ARG=""
    AGENT_ID=$(docker exec "$container" printenv AGENT_ID)
    if [ ! -z "$AGENT_ID" ]; then
        # Trim whitespace
        AGENT_ID=$(echo "$AGENT_ID" | tr -d '\r')
        AGENT_ID_ARG="$AGENT_ID"
    fi
    
    # Construct full script path
    SCRIPT_NAME=$(basename "$script")
    FULL_SCRIPT_PATH="$FLYCHAMS_PATH/tools/run/$SCRIPT_NAME"

    # Run script in container without -d flag, in background, prefixing output with container name
    (
        docker exec "$container" bash -c "$FULL_SCRIPT_PATH $AGENT_ID_ARG True" 2>&1 | \
        while IFS= read -r line; do
            echo "[$container] $line"
        done
    ) &
    
    # Store the background process PID
    bg_pids+=($!)
}

# Check for global container
if docker ps --format '{{.Names}}' | grep -q "^flychams-global$"; then
    run_in_container "flychams-global" "run_global.sh"
else
    echo "Warning: flychams-global container not found running."
fi

# Check for agent containers
agents=$(docker ps --format '{{.Names}}' | grep "^flychams-AGENT" | sort -V)

if [ -z "$agents" ]; then
    echo "Warning: No flychams-agent containers found running."
else
    for container in $agents; do
        run_in_container "$container" "run_agent.sh"
    done
fi

# Wait a moment for processes to start
sleep 1

# Wait for all background processes (this will keep the script alive)
wait