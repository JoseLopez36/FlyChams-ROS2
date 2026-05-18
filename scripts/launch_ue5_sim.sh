#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <path/to/FlyChams-Sim-UE5>"
    exit 1
fi

UE5_PATH="$1"
shift
SETTINGS_FILE="$PROJECT_ROOT/src/flychams_common/config/generated/airsim.json"

if [ ! -f "$SETTINGS_FILE" ]; then
    echo "Error: AirSim settings not found at $SETTINGS_FILE"
    echo "Run: scripts/launch_settings.sh"
    exit 1
fi

# Optimization flags (opt-in via environment variables)
EXTRA_FLAGS=()

[ "${OFFSCREEN:-false}" = "true" ]         && EXTRA_FLAGS+=("-RenderOffScreen")
[ "${NO_SOUND:-false}" = "true" ]           && EXTRA_FLAGS+=("-NoSound")
[ "${UNATTENDED:-false}" = "true" ]         && EXTRA_FLAGS+=("-Unattended")
[ "${NO_SPLASH:-false}" = "true" ]          && EXTRA_FLAGS+=("-nosplash")
[ "${NO_TEXTURE_STREAMING:-false}" = "true" ] && EXTRA_FLAGS+=("-NoTextureStreaming")
[ "${NULL_RHI:-false}" = "true" ]           && EXTRA_FLAGS+=("-NullRHI")

"$UE5_PATH/FlyChamsSim.sh" -settings="$SETTINGS_FILE" "${EXTRA_FLAGS[@]}" "$@"