#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")/.."

# Source the environment file
if [ -f "$ROOT_DIR/docker/config.env" ]; then
    source "$ROOT_DIR/docker/config.env"
fi

# Default to rviz
CMD="run_rviz.sh"

# Parse args for plotjuggler
if [[ "$1" == "--plotjuggler" ]]; then
    CMD="run_plotjuggler.sh"
fi

# Inside container path
CONTAINER_CMD="/home/${USER_NAME}/FlyChams-ROS2/tools/run/$CMD"

echo "Starting rviz container running $CMD..."

docker run --rm -it \
    --name flychams-rviz \
    --network host \
    --privileged \
    -e DISPLAY=$DISPLAY \
    -e QT_X11_NO_MITSHM=1 \
    -e ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0} \
    -e FLYCHAMS_PATH=/home/${USER_NAME}/FlyChams-ROS2 \
    -e AIRSIM_PATH=/home/${USER_NAME}/FlyChams-Cosys-AirSim \
    -e PX4_PATH=/home/${USER_NAME}/PX4-Autopilot \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v "$FLYCHAMS_ROS2_PATH":/home/${USER_NAME}/FlyChams-ROS2 \
    -v "$FLYCHAMS_AIRSIM_PATH":/home/${USER_NAME}/FlyChams-Cosys-AirSim \
    -v "$FLYCHAMS_PX4_PATH":/home/${USER_NAME}/PX4-Autopilot \
    flychams-ros2:latest \
    bash -c "source /opt/ros/iron/setup.bash && source /home/${USER_NAME}/FlyChams-ROS2/ros2_ws/install/setup.bash && $CONTAINER_CMD"