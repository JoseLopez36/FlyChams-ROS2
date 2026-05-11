#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AGENT_ID="${1:-AGENT00}"

CMD="source install/setup.bash && ros2 launch flychams_agent agent.launch.py" \
    "$SCRIPT_DIR/docker/run_agent.sh" "$AGENT_ID"