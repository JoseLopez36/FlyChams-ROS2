# Flying Chameleons ROS2: Multi-UAV System for Autonomous Target Tracking

A ROS2-based system for coordinated multi-UAV target tracking using various advanced simulation frameworks (e.g. AirSim, Unreal Engine 5, etc.).

## Overview

The Flying Chameleons (FlyChams) project implements a complete system for controlling and coordinating multiple UAVs equipped with modifiable tracking systems. The primary goal is to optimize target tracking through collaborative agent positioning and camera control.

The project leverages:
- **Unreal Engine 5** for photorealistic simulation
- **AirSim** for high-fidelity physics simulation
- **PX4** for commercial flight control
- **ROS2 Humble** for the distributed robotics framework
- **Pixi** for dependency and environment management

## Key Features

- Multi-agent coordination for optimal target coverage
- Independent control of multiple cameras per agent
- Clustering algorithms for grouping and tracking targets
- Real-time visualization and monitoring tools via operator interface GUI
- Configurable missions via Excel configuration files
- Realistic simulation in photorealistic Unreal Engine environments
- Interactive operator interface with map visualization and camera streams

## System Architecture

| Package                 | Description                                     |
| ----------------------- | ----------------------------------------------- |
| `flychams_core`         | Core domain models, utilities, and interfaces   |
| `flychams_operator`     | Operator interface GUI and visualization tools |
| `flychams_coordinator`  | Perception algorithms for clustering targets and agent assignment |
| `flychams_agent`        | Agent control, tracking, and positioning       |
| `flychams_simulation`   | Simulation framework manager and target control |
| `flychams_interfaces`   | Custom messages and services for FlyChams      |
| `airsim_wrapper`        | ROS2 interface to the AirSim simulator         |
| `airsim_interfaces`     | Custom messages and services for AirSim         |

## Prerequisites

### Software Requirements

- **Ubuntu 20.04, 22.04, or 24.04** (or compatible Linux distribution)
- **Pixi** (for dependency management) - Install from [pixi.sh](https://pixi.sh)
- **Docker** (optional, for agent containers and PX4 SITL)
- **Unreal Engine 5.2.1** (optional, for developing new environments)

### Hardware Requirements

- **Recommended CPU**: Medium to high-end CPU (e.g. Intel i7-12700K or AMD Ryzen 7 5800X)
- **Recommended GPU**: Medium to high-end GPU with latest drivers (e.g. NVIDIA RTX 3070 or AMD RX 6800 XT)
- **Minimum RAM**: 16 GB

## Installation

### 1. Install Pixi

Install Pixi using the official installer:

```bash
curl -fsSL https://pixi.sh/install.sh | sh
```

Or follow the installation instructions at [pixi.sh](https://pixi.sh).

### 2. Clone the FlyChams repositories

```bash
# Clone FlyChams-ROS2
git clone https://github.com/JoseLopez36/FlyChams-ROS2.git

# Clone FlyChams-Cosys-AirSim
git clone https://github.com/JoseLopez36/FlyChams-Cosys-AirSim.git
git checkout 5.2.1
```

### 3. Setup the PX4-Autopilot repository (for simulation)

```bash
git clone --recursive https://github.com/PX4/PX4-Autopilot.git
cd PX4-Autopilot
# We recommend using the 1.12.0 stable release
git checkout v1.12.0
# Build SITL in docker container
./Tools/docker_run.sh 'make px4_sitl_default none_iris'
```

### 4. Setup the UE5 project

You need to have an Unreal Engine 5 project with the FlyChams-Cosys-AirSim plugin installed. You can find exported projects in the [FlyChams-Sim-UE5](https://github.com/JoseLopez36/FlyChams-Sim-UE5) repository releases.

### 5. Setup environment variables

Edit the `setup.sh` file in the repository root to configure environment variables:

```bash
# Paths (example)
export FLYCHAMS_ROS2_PATH=${HOME}/Documents/FlyChams-ROS2
export FLYCHAMS_PX4_PATH=${HOME}/Documents/PX4-Autopilot
export FLYCHAMS_AIRSIM_PATH=${HOME}/Documents/FlyChams-Cosys-AirSim
export FLYCHAMS_UE5_PATH=${HOME}/Documents/FlyChams-Sim-UE5/Linux

# ROS2 config
export ROS_DOMAIN_ID=0
```

### 6. Install Pixi environment and build the system

The project uses Pixi for managing ROS2 dependencies and build environments. Install the environment:

```bash
pixi install
```

This will automatically install all required dependencies including ROS2 Humble, build tools and Python packages.

### 7. Build the ROS2 workspace

Build the ROS2 packages using Pixi:

```bash
# Build operator package (includes GUI dependencies)
pixi run operator-build

# Build coordinator package
pixi run coordinator-build

# Build simulation package (includes AirSim wrapper)
pixi run simulation-build
```

### 8. Generate AirSim settings

Generate AirSim settings from the configuration:

```bash
pixi run generate-settings
```

This will create settings files based on your mission configuration.

## Usage

The system can be launched in two ways:

### Method 1: Operator Interface (Recommended)

The operator interface provides a graphical user interface for launching and monitoring the system. This is the recommended method for most users.

#### 1. Launch the Unreal Engine Simulation

First, launch the Unreal Engine simulation:

```bash
pixi run simulation-ue5-run
```

#### 2. Launch the Operator Interface

Launch the operator interface GUI:

```bash
pixi run operator-sim-run

# Or
pixi run operator-hardware-run
```

The operator interface provides:

- **Launch Panel**: Buttons to launch system components. It has an integrated terminal output for each launched component
- **Map Panel**: Real-time visualization of agents, targets and clusters on a 2D map
- **Camera Panel**: Live camera streams from all agent cameras

#### 3. Launch System Components via GUI

Using the operator interface:

1. **Launch Coordinator**: Click "LAUNCH COORDINATOR" to start the global coordination system
2. **Launch Simulation**: Click "LAUNCH SIMULATION" to start the simulation nodes (AirSim, target control, etc.)
3. **Launch Agents**: Click "LAUNCH {AGENT_ID}" buttons for each agent you want to run
4. **Launch PX4**: Click "PX4-{INDEX}" buttons to launch PX4 SITL instances for simulation (one per agent)

The operator interface automatically discovers agents, targets and clusters as they register with the system.

#### 4. Stop the System

Click the "STOP" button in the operator interface to stop every process.

### Method 2: Pixi Run Commands

For command-line users or automated scripts, you can launch components directly using Pixi tasks:

#### 1. Launch the Unreal Engine Simulation

```bash
pixi run simulation-ue5-run
```

#### 2. Launch System Components

```bash
# Launch coordinator (simulation mode)
pixi run coordinator-sim-run

# Launch coordinator (hardware mode)
pixi run coordinator-hardware-run

# Launch simulation nodes
pixi run simulation-run

# Launch operator nodes (simulation mode)
pixi run operator-sim-run

# Launch operator nodes (hardware mode)
pixi run operator-hardware-run

# Launch PX4 SITL (requires agent index)
pixi run simulation-px4-run 0
```

#### 3. Launch Agents

For simulation mode, agents can be launched using Docker containers:

```bash
# Build agent Docker image
pixi run agent-sim-build-image

# Build agent workspace
pixi run agent-sim-build

# Run agent (requires agent ID)
pixi run agent-sim-run AGENT00

# Stop agent (requires agent ID)
pixi run agent-sim-stop AGENT00
```

## Configuration

The system is configured using YAML files in the `config/` directory:

- **`config/core/`**: Core system configuration (system.yaml, topics.yaml, frames.yaml, launch.yaml)
- **`config/package/`**: Package-specific configuration (operator.yaml, coordinator.yaml, agent.yaml, simulation.yaml)
- **`config/generated/`**: Generated mission configuration (mission.yaml) - created from Excel configuration
- **`config/rviz/`**: RViz visualization configuration

### Mission Configuration

Missions are configured using Excel spreadsheets in `config/`. The system supports:

- **Mission** - General mission characteristics and selection
- **Environment** - Environment definitions
- **Target** - Target definitions and trajectories (CSV files in `config/trajectories/`)
- **Agent** - Agent configurations
- **Tracking** - Tracking system settings
- **Head** - Head (Gimbal/Camera) specifications
- **Drone** - UAV model specifications
- **Gimbal** - Gimbal model specifications
- **Camera** - Camera model specifications

After editing the Excel configuration, regenerate the mission YAML:

```bash
pixi run generate-settings
```

## Directory Structure

```
FlyChams-ROS2/
├── config/                         # Configuration files
│   ├── core/                      # Core system configuration
│   ├── package/                   # Package-specific configuration
│   ├── generated/                 # Generated mission configuration
│   ├── rviz/                      # RViz configuration
│   ├── trajectories/              # Trajectory CSV files for targets
│   └── Configuration-*.xlsx       # Excel configuration files
├── launch/                        # ROS2 launch files
│   ├── operator.launch.py         # Operator interface launcher
│   ├── coordinator.launch.py      # Coordinator launcher
│   ├── agent.launch.py            # Agent launcher
│   ├── simulation.launch.py       # Simulation launcher
│   └── generate_settings.launch.py # Settings generator
├── src/                           # Source packages
│   ├── flychams_core/             # Core domain models, utilities and interfaces
│   ├── flychams_coordinator/      # Target perception and clustering
│   ├── flychams_agent/            # Agent control and tracking
│   ├── flychams_simulation/       # Simulation manager
│   ├── flychams_operator/         # Operator interface GUI
│   ├── flychams_interfaces/       # Custom messages and services
│   ├── airsim_interfaces/         # AirSim messages and services
│   └── airsim_wrapper/            # ROS2 AirSim wrapper
├── tools/                         # Utility scripts
│   ├── agent_setup.sh             # Agent Docker setup script
│   ├── launch_ue5.sh              # UE5 launcher
│   └── docker/                    # Docker-related scripts
├── build/                         # Build artifacts
├── install/                       # Installed packages
├── log/                           # Build and runtime logs
├── pixi.toml                      # Pixi configuration
├── setup.sh                       # Environment setup script
└── README.md                      # This file
```

## Pixi Tasks Reference

The `pixi.toml` file defines various tasks for building and running the system:

### Build Tasks

- `operator-build`: Build the operator package
- `coordinator-build`: Build the coordinator package
- `simulation-build`: Build the simulation package and AirSim wrapper
- `clean`: Clean build artifacts

### Launch Tasks

- `operator-run`: Launch the operator interface GUI
- `coordinator-sim-run`: Launch coordinator in simulation mode
- `coordinator-hardware-run`: Launch coordinator in hardware mode
- `simulation-run`: Launch simulation nodes
- `simulation-ue5-run`: Launch Unreal Engine 5
- `simulation-px4-run`: Launch PX4 SITL

### Agent Tasks (Docker-based)

- `agent-sim-build-image`: Build agent Docker image
- `agent-sim-build`: Build agent workspace in Docker
- `agent-sim-run`: Run agent in Docker container
- `agent-sim-stop`: Stop agent container
- `agent-sim-remove`: Remove agent container
- `agent-sim-shell`: Open shell in agent container

### Utility Tasks

- `generate-settings`: Generate AirSim settings from configuration

## Known Limitations

1. **Performance Limitations**
   - The system has been tested with a single UAV agent, with up to 4 cameras.
   - Performance may degrade significantly with more elements.

2. **AirSim Integration**
   - Currently requires a specific fork of AirSim with custom modifications.
   - Limited support for AirSim's newer features.

3. **Real Hardware Integration**
   - Current implementation focuses on simulation; real hardware integration is available but may require additional configuration.

4. **Operator Interface**
   - Requires X11 forwarding or a display server for GUI components.
   - Camera streams require proper network configuration for video streaming.

## Troubleshooting

### Operator Interface Not Launching

- Ensure X11 forwarding is enabled if running remotely
- Check that PyQt5 dependencies are installed: `pixi install`
- Verify ROS2 environment is sourced: `source install/setup.bash`

### Build Errors

- Clean and rebuild: `pixi run clean && pixi run operator-build`
- Check that all dependencies are installed: `pixi install`
- Verify Python executable paths in CMake configuration

### Agent Launch Issues

- Ensure Docker is running: `docker ps`
- Check agent container logs: `docker logs <container_name>`
- Verify PX4 is running before launching agents in simulation mode

## License

This project is licensed under the [Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)](https://creativecommons.org/licenses/by-nc/4.0/). See [`LICENSE`](LICENSE).

## Contact

For more information, please contact [josloprui6@alum.us.es](mailto:josloprui6@alum.us.es).
