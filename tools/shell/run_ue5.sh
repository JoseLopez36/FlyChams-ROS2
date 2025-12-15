#!/bin/bash

cd $FLYCHAMS_SIM_UE5_PATH || { echo "Directory $FLYCHAMS_SIM_UE5_PATH was not found."; exit 1; }

source "$FLYCHAMS_ROS2_PATH/docker/config.env"

echo "Running UE5 simulation..."

./FlyChamsSim.sh -settings="$FLYCHAMS_ROS2_PATH/config/settings.json"

echo "UE5 simulation running successfully"