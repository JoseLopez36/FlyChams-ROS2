#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AGENT_ID="${1:-AGENT00}"

CMD="colcon build --packages-select flychams_api flychams_common flychams_agent" \
    "$SCRIPT_DIR/docker/run_agent.sh" "$AGENT_ID"