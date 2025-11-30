#!/bin/bash

cd $AIRSIM_PATH || { echo "Directory $AIRSIM_PATH was not found."; exit 1; }

echo "Cleaning AirSim dependencies..."

./clean.sh

echo "AirSim dependencies cleaned successfully"