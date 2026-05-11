#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CMD="colcon build --packages-select flychams_api flychams_common flychams_simulation" \
    "$SCRIPT_DIR/docker/run_simulation.sh"