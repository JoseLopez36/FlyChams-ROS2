#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AGENT_ID="${1:-AGENT00}"

CMD="source /opt/ros/humble/setup.bash && source /home/testuser/px4_msgs_ws/install/setup.bash && colcon build --packages-select flychams_api flychams_common flychams_agent" \
    "$SCRIPT_DIR/docker/run_agent.sh" "$AGENT_ID"