#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DETACH="${DETACH:-false}"
XRCE_PORT="${XRCE_PORT:-8888}"

CMD="MicroXRCEAgent udp4 -p ${XRCE_PORT}" \
    DETACH="$DETACH" "$SCRIPT_DIR/docker/run_micro_xrce_dds.sh"