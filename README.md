# Flying Chameleons ROS2: Multi-UAV System for Autonomous Target Tracking

A ROS2-based system for coordinated multi-UAV target tracking using various advanced simulation frameworks (e.g. AirSim, Unreal Engine 5, etc.).

## Overview

The Flying Chameleons (FlyChams) project implements a complete system for controlling and coordinating multiple UAVs equipped with modifiable tracking systems. The primary goal is to optimize target tracking through collaborative agent positioning and camera control.

The project leverages:
- **Unreal Engine 5** for photorealistic simulation
- **AirSim** for high-fidelity physics simulation
- **PX4** for commercial flight control
- **ROS2** for the distributed robotics framework

## Key Features

- Multi-agent coordination for optimal target coverage
- Independent control of multiple cameras per agent
- Clustering algorithms for grouping and tracking targets
- Real-time visualization and monitoring tools
- Configurable missions via Excel configuration files
- Realistic simulation in photorealistic Unreal Engine environments

## System Architecture

| Package                 | Description                                     |
| ----------------------- | ----------------------------------------------- |
| `flychams_core`         | Core domain models, utilities, and interfaces   |
| `flychams_bringup`      | Launch files and configuration for the system   |
| `flychams_control`      | Control algorithms for aerial agents            |
| `flychams_perception`   | Perception algorithms for clustering targets    |
| `flychams_coordination` | Coordination algorithms for multi-agent systems |
| `flychams_simulation`   | Manager for simulation framework                |
| `flychams_interfaces`   | Custom message and service for FlyChams         |
| `airsim_wrapper`        | ROS2 interface to the AirSim simulator          |
| `airsim_interfaces`     | Custom message and service for AirSim           |

## Prerequisites

### Software Requirements

- **Ubuntu 20.04, 22.04, or 24.04** (or compatible Linux distribution)
- **Docker** (for running the system in a container)
- **Unreal Engine 5.2.1** (optional, for developing new environments)

### Hardware Requirements

- **Recommended CPU**: Medium to high-end CPU (e.g. Intel i7-12700K or AMD Ryzen 7 5800X)
- **Recommended GPU**: Medium to high-end GPU with latest drivers (e.g. NVIDIA RTX 3070 or AMD RX 6800 XT)
- **Minimum RAM**: 16 GB

## Installation

### 1. Clone the FlyChams repositories

**Host Machine**
```bash
git clone https://github.com/JoseLopez36/FlyChams-ROS2.git
git clone https://github.com/JoseLopez36/FlyChams-Cosys-AirSim.git
# Checkout to branch 5.2.1
cd FlyChams-Cosys-AirSim
git checkout 5.2.1
```

### 2. Setup the PX4-Autopilot repository

**Host Machine**
```bash
git clone --recursive https://github.com/PX4/PX4-Autopilot.git
cd PX4-Autopilot/
# We recommend using the 1.12.0 stable release
git checkout v1.12.0
# Build SITL in docker container
./Tools/docker_run.sh 'make px4_sitl_default none_iris'
```
*Note: 172.17.0.1 is the IP address of the host machine from the docker container. Check this corresponds to the IP address of the host machine. If you are running PX4 directly on the host machine, you can use 127.0.0.1.*

### 3. Setup the UE5 project

You need to have an UE project with the FlyChams-Cosys-AirSim plugin installed. You can find exported projects in the `FlyChams-Sim-UE5` repository releases.

### 4. Setup environment variables

Configure the environment variables in the `docker/config.env` file. Edit these variables to match your local paths:

**Host Machine**
```bash
# Shared paths
FLYCHAMS_ROS2_PATH=${HOME}/Documents/FlyChams-ROS2
FLYCHAMS_AIRSIM_PATH=${HOME}/Documents/FlyChams-Cosys-AirSim
FLYCHAMS_PX4_PATH=${HOME}/Documents/PX4-Autopilot
FLYCHAMS_SIM_UE5_PATH=${HOME}/Documents/FlyChams-Sim-Coastal
```

### 5. Build the docker image

Run the following command to build the docker image:

**Host Machine**
```bash
./docker/build.sh
```
*Note: This will build the docker image. It may take a while to build the image.*

### 6. Setup and build the system

Run the following command to setup dependencies, build the workspace and generate settings:

**Host Machine**
```bash
./tools/docker/run_build_container.sh --regenerate-airsim
```
*Note: This will launch a temporary container to perform all build and setup operations.*

## Usage

### 1. Launch the Unreal Engine Simulation

Run the following command to launch the Unreal Engine simulation with the previously generated AirSim settings:

**Host Machine**
```bash
./path/to/FlyChams-ROS2/tools/airsim/run_ue5.sh
```

### 2. Setup the docker containers

Run the following command to generate the container structure and setup the system:

**Host Machine**
```bash
./tools/docker/setup_simulation_containers.sh --agents <number_of_agents>
```
*Note: This will generate a docker-compose.yml file and start the containers (one global container + one container per agent). Replace <number_of_agents> with the number of agents you want to simulate.*

### 3. Run the docker containers

Run the following command to run each container:

**Host Machine**
```bash
./tools/docker/run_simulation_containers.sh
```

The ROS2 system is launched:

- **Global nodes** are launched in the `flychams-global` container.
- **Agent nodes** are launched in their respective `flychams-agent-k` containers.

You can check the logs of a specific container:

**Host Machine**
```bash
docker logs -f flychams-global
# or
docker logs -f flychams-agent-k
```

To stop and remove the containers, you can use the following command:

**Host Machine**
```bash
./tools/docker/stop_simulation_containers.sh
```

### 4. (Optional) Visualization

To view the system in RViz:

**Host Machine**
```bash
./tools/docker/run_rviz_container.sh
```

To plot simulation data on runtime, we recommend using `PlotJuggler`. To run it, use the following command:

**Host Machine**
```bash
./tools/docker/run_rviz_container.sh --plotjuggler
```

You can also plot previous rosbag data by importing them into the PlotJuggler window. More info [here](https://plotjuggler.io/). To record rosbags you must configure it in the `Configuration.xlsx` file and use the following command:

**Docker**
```bash
ros2 launch flychams_bringup rosbag.launch.py
```

## Configuration

The system is mainly configured using an Excel spreadsheet (`Configuration.xlsx`). This file includes:

1. **Mission** - General mission characteristics and selection
2. **Environment** - Environment definitions
3. **Target** - Target definitions and trajectories
4. **Agent** - Agent configurations
5. **Tracking** - Tracking system settings
6. **Head** - Head (Gimbal/Camera) specifications
7. **Drone** - UAV model specifications
8. **Gimbal** - Gimbal model specifications
9. **Camera** - Camera model specifications

## Directory Structure

```
FlyChams-ROS2/
├── config/                         # Configuration files
│   └── Configuration.xlsx          # Main configuration spreadsheet
│   └── Trajectories/               # Trajectory files for targets
├── ros2_ws/                        # ROS2 workspace
│   └── src/                        # Source packages
│       ├── flychams_core/          # Core domain models, utilities and interfaces
│       ├── flychams_bringup/       # Launch and configuration
│       ├── flychams_control/       # Agent control
│       ├── flychams_perception/    # Target perception
│       ├── flychams_coordination/  # Multi-agent coordination
│       ├── flychams_simulation/    # Simulation manager
│       ├── flychams_interfaces/    # Custom message and service for FlyChams
│       ├── airsim_interfaces/      # Custom message and service for AirSim wrapper
│       └── airsim_wrapper/         # ROS2 AirSim wrapper
├── experiments/                    # Experiment data and settings
├── docker/                         # Docker files and scripts
└── tools/                          # Utility scripts and tools
```

## Known Limitations

1. **Performance Limitations**
   - The system has been tested with a single UAV agent, with up to 4 cameras.
   - Performance may degrade significantly with more elements.

2. **AirSim Integration**
   - Currently requires a specific fork of AirSim with custom modifications.
   - Limited support for AirSim's newer features.

3. **Real Hardware Integration**
   - Current implementation focuses on simulation; real hardware integration will be available in future releases.

## Contact

For more information, please contact [josloprui6@alum.us.es](mailto:josloprui6@alum.us.es).
