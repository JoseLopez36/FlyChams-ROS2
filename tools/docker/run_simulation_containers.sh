#!/bin/bash

# Function to run command in container with persistent stdin to keep script alive
run_in_container() {
    local container=$1
    local script=$2
    
    echo "Starting simulation in $container..."
    
    # Get FLYCHAMS_PATH from container environment
    FLYCHAMS_PATH=$(docker exec "$container" printenv FLYCHAMS_PATH)
    if [ -z "$FLYCHAMS_PATH" ]; then
        echo "Error: FLYCHAMS_PATH not set in $container. Using default."
        FLYCHAMS_PATH="/home/testuser/FlyChams-ROS2"
    fi
    
    # Trim whitespace
    FLYCHAMS_PATH=$(echo "$FLYCHAMS_PATH" | tr -d '\r')
    
    # Construct full script path
    SCRIPT_NAME=$(basename "$script")
    FULL_SCRIPT_PATH="$FLYCHAMS_PATH/tools/run/$SCRIPT_NAME"

    # Run script in container
    docker exec -d "$container" bash -c "$FULL_SCRIPT_PATH < <(tail -f /dev/null)"
}

# Check for global container
if docker ps --format '{{.Names}}' | grep -q "^flychams-global$"; then
    run_in_container "flychams-global" "run_global.sh"
else
    echo "Warning: flychams-global container not found running."
fi

# Check for agent containers
agents=$(docker ps --format '{{.Names}}' | grep "^flychams-agent-" | sort -V)

if [ -z "$agents" ]; then
    echo "Warning: No flychams-agent containers found running."
else
    for container in $agents; do
        run_in_container "$container" "run_agent.sh"
    done
fi

echo "Commands sent to containers."

