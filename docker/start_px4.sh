#!/bin/bash
# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Source the environment file relative to the script location
source "$SCRIPT_DIR/config.env"

# Get number of instances from first argument, default to 1
N=${1:-1}

# Launch PX4 SITL
# First build, then run multiple instances
$FLYCHAMS_PX4_PATH/Tools/docker_run.sh "\
    export PX4_SIM_HOSTNAME=172.17.0.1 && \
    make px4_sitl_default && \
    ./Tools/sitl_multiple_run.sh $N && \
    echo 'PX4 instances started. Type exit to stop the container.' && /bin/bash"