#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AGENT_ID="${1:-AGENT00}"
DETACH="${DETACH:-false}"
FCU_URL="${FCU_URL:-}"
TGT_SYSTEM="${TGT_SYSTEM:-}"

AGENT_LAUNCH_ARGS="agent_id:=${AGENT_ID}"

MAVROS_LAUNCH_ARGS="agent_id:=${AGENT_ID} log_level:=error"
[ -n "$FCU_URL" ]    && MAVROS_LAUNCH_ARGS="$MAVROS_LAUNCH_ARGS fcu_url:=${FCU_URL}"
[ -n "$TGT_SYSTEM" ] && MAVROS_LAUNCH_ARGS="$MAVROS_LAUNCH_ARGS tgt_system:=${TGT_SYSTEM}"

CMD="source install/setup.bash && ros2 launch flychams_agent mavros.launch.py ${MAVROS_LAUNCH_ARGS} & source install/setup.bash && ros2 launch flychams_agent agent.launch.py ${AGENT_LAUNCH_ARGS}" \
    DETACH="$DETACH" "$SCRIPT_DIR/docker/run_agent.sh" "$AGENT_ID"