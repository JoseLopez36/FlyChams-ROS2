#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AGENT_IDX="${1:-}"
if [ -z "$AGENT_IDX" ]; then
    echo "Usage: $0 <AGENT_IDX>"
    echo "Example: $0 0"
    exit 1
fi

DETACH="${DETACH:-false}"

CMD="PX4_SIM_HOSTNAME=172.17.0.1 PX4_SIM_MODEL=iris ${FLYCHAMS_PX4_PATH}/build/px4_sitl_default/bin/px4 -i ${AGENT_IDX} -d ${FLYCHAMS_PX4_PATH}/ROMFS/px4fmu_common -s etc/init.d-posix/rcS" \
    DETACH="$DETACH" "$SCRIPT_DIR/docker/run_px4.sh" "$AGENT_IDX"