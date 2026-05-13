#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <path/to/FlyChams-Sim-UE5>"
    exit 1
fi

UE5_PATH="$1"
SETTINGS_FILE="$PROJECT_ROOT/src/flychams_common/config/generated/airsim.json"

if [ ! -f "$SETTINGS_FILE" ]; then
    echo "Error: AirSim settings not found at $SETTINGS_FILE"
    echo "Run: scripts/launch_settings.sh"
    exit 1
fi

"$UE5_PATH/FlyChamsSim.sh" -settings="$SETTINGS_FILE"