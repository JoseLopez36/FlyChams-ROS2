#!/bin/bash

source "$FLYCHAMS_ROS2_PATH/docker/config.env"

cd $FLYCHAMS_SIM_UE5_PATH || { echo "Directory $FLYCHAMS_SIM_UE5_PATH was not found."; exit 1; }

echo "Running UE5 simulation..."

./FlyChamsSim.sh -settings="$FLYCHAMS_ROS2_PATH/config/airsim.json"

echo "UE5 simulation running successfully"