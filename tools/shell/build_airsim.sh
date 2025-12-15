#!/bin/bash

cd $AIRSIM_PATH || { echo "Directory $AIRSIM_PATH was not found."; exit 1; }

echo "Building AirSim dependencies..."

./clean.sh
./setup.sh
./build.sh

echo "AirSim dependencies built successfully"