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
| `flychams_coordinator`   | Perception algorithms for clustering targets    |
| `flychams_coordination` | Coordination algorithms for multi-agent systems |
| `flychams_simulation`   | Manager for simulation framework                |
| `flychams_interfaces`   | Custom message and service for FlyChams         |
| `airsim_wrapper`        | ROS2 interface to the AirSim simulator          |
| `airsim_interfaces`     | Custom message and service for AirSim           |

## Prerequisites

### Software Requirements

- **Ubuntu 20.04, 22.04, or 24.04** (or compatible Linux distribution)
- **Docker** (for running the system in a container)
- **Tmux** (for managing terminal sessions)
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

Configure the environment variables in the `.env` file. Edit these variables to match your local paths:

**Host Machine**
```bash
# Paths
FLYCHAMS_ROS2_PATH=${HOME}/Documents/FlyChams-ROS2
FLYCHAMS_AIRSIM_PATH=${HOME}/Documents/FlyChams-Cosys-AirSim
FLYCHAMS_PX4_PATH=${HOME}/Documents/PX4-Autopilot
FLYCHAMS_UE5_PATH=${HOME}/Documents/FlyChams-Sim-Coastal
```

### 5. Build the docker image

Run the following command to build the docker image:

**Host Machine**
```bash
./tools/docker/build_image.sh
```
*Note: This will build the docker image. It may take a while to build the image.*

### 6. Install Python dependencies

Run the following command to install the required Python packages:

**Host Machine**
```bash
sudo apt install python3-libtmux
```

### 7. Setup and build the system

Build the ROS2 workspace and generate AirSim settings:

**Host Machine**
```bash
# Build AirSim dependencies (in Docker container)
python3 ./tools/launch_build.py --build-airsim

# Build ROS2 workspace (in Docker container)
python3 ./tools/launch_build.py --build-ros2 -j 2

# Generate AirSim settings (in Docker container)
python3 ./tools/launch_build.py --generate-settings
```

Alternatively, you can build on the host machine directly:
```bash
# Build AirSim dependencies (on host)
python3 ./tools/launch_build.py --build-on-host --build-airsim

# Build ROS2 workspace (on host)
python3 ./tools/launch_build.py --build-on-host --build-ros2 -j 2

# Generate AirSim settings (on host)
python3 ./tools/launch_build.py --build-on-host --generate-settings
```

*Note: The `-j` flag specifies the number of parallel threads for building. Default is 2. Use `--build-on-host` to build directly on the host machine instead of in a Docker container.*

## Usage

### 1. Launch the Unreal Engine Simulation

Run the following command to launch the Unreal Engine simulation with the previously generated AirSim settings:

**Host Machine**
```bash
./tools/shell/run_ue5.sh
```

### 2. Launch the System

Run the following command to launch the system:

**Host Machine**
```bash
# Launch in simulation mode
python3 ./tools/launch.py --sim

# Launch in hardware mode
python3 ./tools/launch.py --hardware

# Launch with custom delay between setup and run (default: 1.0 seconds)
python3 ./tools/launch.py --sim --delay 2.0
```

*Note: This script creates a tmux session named `flychams` and automatically launches all necessary components. It creates windows for:*
- *GLOBAL: Global coordination tasks*
- *AGENTxx: One window per agent (from config/mission.yaml)*
- *PX4-i: PX4 SITL instances (simulation mode only, one per agent)*
- *VISUALIZATION: RViz visualization*

*You can navigate between windows using `Ctrl+B` then `n` (next) or `p` (previous), and detach from the session with `Ctrl+B` then `d`. To reattach, use `tmux attach -t flychams`. Press `Ctrl+K` to stop the session (with confirmation).*

To stop the system, use:
```bash
# Stop simulation mode
python3 ./tools/stop.py --sim

# Stop hardware mode
python3 ./tools/stop.py --hardware
```
*This will kill the tmux session and stop all FlyChams containers/processes.*

### 3. (Optional) Visualization

Launch visualization tools:

**Host Machine**
```bash
# Launch RViz
python3 ./tools/launch_visualization.py

# Launch PlotJuggler
python3 ./tools/launch_visualization.py --plotjuggler
```

*Note: Visualization tools run in Docker containers with X11 forwarding enabled. Make sure your X11 server is properly configured.*

**Docker**
```bash
ros2 launch flychams_bringup rosbag.launch.py
```

## Tools Reference

The `tools/` directory contains Python scripts and shell scripts for building, launching, and managing the FlyChams system:

### Build Tools

- **`launch_build.py`**: Build ROS2 workspace, AirSim dependencies, or generate AirSim settings
  - `--build-ros2`: Build ROS2 workspace
  - `--build-airsim`: Build AirSim dependencies
  - `--generate-settings`: Generate AirSim settings from configuration
  - `--build-on-host`: Build on host machine instead of Docker container
  - `-j N`: Number of parallel threads for ROS2 build (default: 2)

### Launch Tools

- **`launch.py`**: Main launcher that creates a tmux session and launches all system components
  - `--sim`: Launch in simulation mode
  - `--hardware`: Launch in hardware mode
  - `--delay SECONDS`: Delay between setup and run stages (default: 1.0)

- **`launch_global.py`**: Launch global coordination instance
  - `--sim`: Simulation mode
  - `--hardware`: Hardware mode
  - `--delay SECONDS`: Delay between setup and run

- **`launch_agent.py`**: Launch individual agent instance
  - `--agent-id ID`: Agent identifier (required)
  - `--sim`: Simulation mode
  - `--hardware`: Hardware mode
  - `--delay SECONDS`: Delay between setup and run

- **`launch_px4.py`**: Launch PX4 SITL instance (simulation only)
  - `--agent-index INDEX`: Agent index (required)

- **`launch_visualization.py`**: Launch visualization tools
  - `--plotjuggler`: Launch PlotJuggler instead of RViz

### Management Tools

- **`stop.py`**: Stop the FlyChams system
  - `--sim`: Stop simulation mode
  - `--hardware`: Stop hardware mode

### Shell Scripts

The `tools/shell/` directory contains shell scripts that are typically called by the Python launchers:

- `build_ros2_ws.sh`: Build ROS2 workspace
- `build_airsim.sh`: Build AirSim dependencies
- `create_settings.sh`: Generate AirSim settings
- `setup_global.sh`: Setup global instance
- `run_global.sh`: Run global instance
- `setup_agent.sh`: Setup agent instance
- `run_agent.sh`: Run agent instance
- `run_rviz.sh`: Launch RViz
- `run_plotjuggler.sh`: Launch PlotJuggler
- `run_ue5.sh`: Launch Unreal Engine 5 simulation
- `docker_run_px4.sh`: Run PX4 in Docker container
- `clean_ros2_ws.sh`: Clean ROS2 workspace

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
│       ├── flychams_coordinator/    # Target perception
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

## License

This project is licensed under the [Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)](https://creativecommons.org/licenses/by-nc/4.0/). See [`LICENSE`](LICENSE).

## Contact

For more information, please contact [josloprui6@alum.us.es](mailto:josloprui6@alum.us.es).
