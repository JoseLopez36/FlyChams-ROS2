#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")/.."

# Source the environment file
if [ -f "$ROOT_DIR/docker/config.env" ]; then
    source "$ROOT_DIR/docker/config.env"
fi

# Parse command line arguments
REGENERATE_AIRSIM=""
for arg in "$@"; do
    case $arg in
        --regenerate-airsim)
            REGENERATE_AIRSIM="--regenerate-airsim"
            shift
            ;;
    esac
done

echo "Starting build container..."
echo "Mounting:"
echo "  - $FLYCHAMS_ROS2_PATH"
echo "  - $FLYCHAMS_AIRSIM_PATH"
echo "  - $FLYCHAMS_PX4_PATH"

docker run --rm -it \
    --name flychams-build \
    --network host \
    --privileged \
    -e DISPLAY=$DISPLAY \
    -e QT_X11_NO_MITSHM=1 \
    -e FLYCHAMS_PATH=/home/${USER_NAME}/FlyChams-ROS2 \
    -e AIRSIM_PATH=/home/${USER_NAME}/FlyChams-Cosys-AirSim \
    -e PX4_PATH=/home/${USER_NAME}/PX4-Autopilot \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v "$FLYCHAMS_ROS2_PATH":/home/${USER_NAME}/FlyChams-ROS2 \
    -v "$FLYCHAMS_AIRSIM_PATH":/home/${USER_NAME}/FlyChams-Cosys-AirSim \
    -v "$FLYCHAMS_PX4_PATH":/home/${USER_NAME}/PX4-Autopilot \
    flychams-ros2:latest \
    bash -c "source /opt/ros/iron/setup.bash && /home/${USER_NAME}/FlyChams-ROS2/tools/build/setup.sh $REGENERATE_AIRSIM"