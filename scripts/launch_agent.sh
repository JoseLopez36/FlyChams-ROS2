#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AGENT_ID="${1:-AGENT00}"
DETACH="${DETACH:-false}"
FCU_URL="${FCU_URL:-}"
TGT_SYSTEM="${TGT_SYSTEM:-}"

LAUNCH_ARGS="agent_id:=${AGENT_ID}"
[ -n "$FCU_URL" ]    && LAUNCH_ARGS="$LAUNCH_ARGS fcu_url:=${FCU_URL}"
[ -n "$TGT_SYSTEM" ] && LAUNCH_ARGS="$LAUNCH_ARGS tgt_system:=${TGT_SYSTEM}"

CMD="source install/setup.bash && ros2 launch flychams_agent agent.launch.py ${LAUNCH_ARGS}" \
    DETACH="$DETACH" "$SCRIPT_DIR/docker/run_agent.sh" "$AGENT_ID"