#!/bin/bash

# Exit on error
set -e

# Parse arguments
REGENERATE_AIRSIM=false

while [[ $# -gt 0 ]]; do
  case $1 in
    --regenerate-airsim)
      REGENERATE_AIRSIM=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

echo "================================================================="
echo " STARTING FULL SETUP AND BUILD"
echo "================================================================="

# 1. Build FlyChams-Cosys-AirSim dependencies
if [ "$REGENERATE_AIRSIM" = true ]; then
  echo ">> [1/3] Building AirSim dependencies..."
  $FLYCHAMS_PATH/tools/airsim/clean_dependencies.sh
  $FLYCHAMS_PATH/tools/airsim/setup_dependencies.sh
  $FLYCHAMS_PATH/tools/airsim/build_dependencies.sh
else
  echo ">> [1/3] Skipping AirSim dependencies (use --regenerate-airsim to rebuild)..."
fi

# 2. Build ROS2 workspace
echo ">> [2/3] Building ROS2 workspace..."
$FLYCHAMS_PATH/tools/build/build.sh -j 3

# 3. Generate AirSim Settings
echo ">> [3/3] Generating AirSim settings..."
$FLYCHAMS_PATH/tools/airsim/create_settings.sh

echo "================================================================="
echo " FULL SETUP COMPLETED SUCCESSFULLY"
echo "================================================================="