#!/bin/bash

cd $AIRSIM_PATH || { echo "Directory $AIRSIM_PATH was not found."; exit 1; }

echo "Setting up AirSim dependencies..."

./setup.sh

echo "AirSim dependencies setup successfully"