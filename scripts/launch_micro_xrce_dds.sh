#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DETACH="${DETACH:-false}"
XRCE_PORT="${XRCE_PORT:-8888}"
XRCE_BAUDRATE="${XRCE_BAUDRATE:-921600}"

if [ -n "${XRCE_SERIAL_DEVICE:-}" ]; then
    CMD="serial --dev ${XRCE_SERIAL_DEVICE} -b ${XRCE_BAUDRATE}"
else
    CMD="udp4 -p ${XRCE_PORT}"
fi

XRCE_SERIAL_DEVICE="${XRCE_SERIAL_DEVICE:-}" \
    CMD="$CMD" \
    DETACH="$DETACH" \
    "$SCRIPT_DIR/docker/run_micro_xrce_dds.sh"