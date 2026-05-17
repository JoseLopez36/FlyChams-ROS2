#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AGENT_ID="${1:-AGENT00}"
DETACH="${DETACH:-false}"

# Use AGENT_IDX from environment variable
if [ -n "$AGENT_IDX" ]; then
    echo "Using AGENT_IDX from environment: $AGENT_IDX"
else
    echo "Warning: AGENT_IDX not set"
fi

AGENT_LAUNCH_ARGS="agent_id:=${AGENT_ID}"

CMD="source /home/testuser/px4_msgs_ws/install/setup.bash && source install/setup.bash && ros2 launch flychams_agent agent.launch.py ${AGENT_LAUNCH_ARGS}" \
    DETACH="$DETACH" "$SCRIPT_DIR/docker/run_agent.sh" "$AGENT_ID"