# Flying Chameleons ROS2: Multi-UAV System for Autonomous Target Tracking

[![ROS2 Humble](https://img.shields.io/badge/ROS2-Humble-blue.svg)](https://docs.ros.org/en/humble/)
[![Unreal Engine 5](https://img.shields.io/badge/Unreal_Engine-5-313131?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![AirSim](https://img.shields.io/badge/AirSim-Enabled-blue)](https://microsoft.github.io/AirSim/)
[![PX4](https://img.shields.io/badge/PX4-Autopilot-black)](https://px4.io/)
[![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc/4.0/)

The Flying Chameleons (FlyChams) project implements a complete system for controlling and coordinating multiple UAVs equipped with modifiable tracking systems. The primary goal is to optimize target tracking through collaborative agent positioning and camera control.

The project leverages:
- **ROS2 Humble** for the distributed robotics framework
- **Unreal Engine 5** for photorealistic simulation
- **AirSim** for high-fidelity physics simulation
- **PX4** for commercial flight control
- **Pixi** for GCS dependency and environment management

---

<div align="center">
  <img src="media/images/MultiWindowSimulation.png" alt="Full Simulation Environment" width="100%"/>
  <p><em>Complete simulation view showing the UE5 environment, operator GUI, and real-time RViZ data.</em></p>
</div>

|  |  |
| :---: | :---: |
| <img src="media/images/MultiWindowViews.png" alt="Target Tracking" width="70%"/> | <img src="media/images/MultiWindowAgent.png" alt="Drone Close-up" width="100%"/> |
| *Real-time multi-target tracking windows* | *High-fidelity hexacopter model in UE5* |

## Key Features

- **Multi-agent coordination** for optimal target coverage
- **Independent control** of multiple cameras per agent
- **Clustering algorithms** for grouping and tracking targets
- **Configurable missions** via Excel configuration files
- **Realistic simulation** in photorealistic Unreal Engine environments
- **Interactive operator interface** with map visualization and camera streams

## Research

This project is part of a broader research initiative by the Department of System Engineering and Automation at the University of Seville. It is associated with the following scientific publications:

1. **Flying Chameleons: A New Concept for Minimum-Deployment, Multiple-Target Tracking Drones**
   
   *Sensors, 2022* | [DOI](https://doi.org/10.3390/s22062359)
   
   > This article introduces the innovative concept of "Flying Chameleons", autonomous aerial vehicles equipped with multiple independently steerable cameras for simultaneous tracking of multiple mobile targets. The proposal seeks to maximize efficiency in surveillance and tracking applications while minimizing resource deployment, offering an alternative to traditional approaches that require multiple vehicles or shared attention strategies.

2. **Optimal Positioning Strategy for Multi-camera, Zooming Drones**
   
   *IEEE/CAA Journal of Automatica Sinica, 2024* | [DOI](https://doi.org/10.1109/JAS.2024.124455)
   
   > This research extends the "Flying Chameleons" concept by incorporating zoom capabilities in the onboard cameras. It addresses the resulting non-convex optimization problem through convex relaxation techniques, allowing the aerial agent to dynamically adjust the focal lengths of the cameras to balance the real distance to targets with the required level of visual detail.

3. **Monitoring through Multi-camera Aerial Vehicles: A Case Study Using Unreal Engine**
   
   *Jornadas de Automática (JJAA) 2024, Málaga* | [DOI](https://doi.org/10.17979/ja-cea.2024.45.10800)
   
   > This work generalizes the multi-camera agent concept to enable collaboration among multiple agents in a single monitoring mission. Additionally, it explores the potential of Unreal Engine 5 as a photorealistic graphical simulation tool for implementing and validating the proposal. Note: This work was selected for presentation among the 6 works chosen at the Jornadas de Automática 2024 in Málaga.

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
- **Pixi** - Required for the Ground Control Station (GCS) to manage the internal ROS2 environment and dependencies. Install from [pixi.sh](https://pixi.sh).
- **Docker** - Required for running agents in isolated containers and for PX4 SITL.
- **Unreal Engine 5.2.1** - Required for running the photorealistic simulation environment.

### Hardware Requirements

#### Simulation Mode (PC)
- **CPU**: Intel i7-12700K / AMD Ryzen 7 5800X or better
- **GPU**: NVIDIA RTX 3070 / AMD RX 6800 XT or better (for UE5)
- **RAM**: 16 GB minimum (32 GB recommended)

#### Hardware Mode (Onboard Computer)
- **Device**: NVIDIA Jetson Orin Nano Super
- **OS**: Ubuntu 22.04 (JetPack 6.0+)

## 📦 Installation

The installation instructions are divided depending on the role of the machine:
1. **Ground Control Station (GCS)**: Typically runs on a standard PC and handles the simulation environment, operator interface, and coordination.
2. **Agents**: Typically runs on a PC for simulation or an NVIDIA Jetson for real deployment.

### 1. Common Prerequisites

**Clone Main Repository**
```bash
git clone https://github.com/JoseLopez36/FlyChams-ROS2.git
cd FlyChams-ROS2
```

---

### 2. Ground Control Station (GCS) Installation

*Follow this section if you are setting up the main simulation and operator machine.*

**Pixi Environment Manager**
The project uses Pixi to manage the internal ROS2 environment and dependencies for the GCS.
```bash
curl -fsSL https://pixi.sh/install.sh | sh
```

**Clone Dependencies**
```bash
# AirSim Plugin (for simulation)
git clone https://github.com/JoseLopez36/FlyChams-Cosys-AirSim.git
cd FlyChams-Cosys-AirSim
git checkout 5.2.1
cd ..

# PX4 Autopilot (for Simulation)
git clone --recursive https://github.com/PX4/PX4-Autopilot.git
cd PX4-Autopilot
git checkout v1.12.0
./Tools/docker_run.sh 'make px4_sitl_default none_iris' # Build SITL
cd ..
```

**Setup Environment**
Edit `setup.sh` to configure your paths:

```bash
export FLYCHAMS_ROS2_PATH=${HOME}/Documents/FlyChams-ROS2
export FLYCHAMS_PX4_PATH=${HOME}/Documents/PX4-Autopilot
export FLYCHAMS_AIRSIM_PATH=${HOME}/Documents/FlyChams-Cosys-AirSim
```

**Install & Build GCS**
```bash
pixi install
pixi run gcs-build
```

**Generate Settings**
Generate the initial AirSim settings based on the configuration:

```bash
pixi run generate-settings
```

---

### 3. Agent Installation

*Follow this section if you are setting up an Agent. If you are running the simulation Agent on the same machine as the GCS, you have already cloned the required repositories and can skip the clone step.*

**NVIDIA Container Toolkit**
Agents run in Docker containers and require GPU access. You **must** install the NVIDIA Container Toolkit to enable GPU support in Docker.
Follow the official guide here: [Installing NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)

**Clone Dependencies (if on a dedicated machine like a Jetson)**
```bash
# AirSim Plugin
git clone https://github.com/JoseLopez36/FlyChams-Cosys-AirSim.git
cd FlyChams-Cosys-AirSim
git checkout 5.2.1
cd ..
```

**Install & Build Agents**
```bash
# Build the base Docker image
bash tools/docker/build_image.sh
# In Jetson, it may be necessary to add the user to the docker group:
# sudo groupadd docker
# sudo usermod -aG docker $USER
# newgrp docker

# Build the agent workspace
bash tools/build_agent.sh
```

## 🚀 Quick Start

The fastest way to get the system running in simulation mode:

1.  **Launch Operator Interface**:
    ```bash
    pixi run operator-sim-run
    ```

2.  **Start System via GUI. Example workflow**:
    - Press **Launch Unreal Engine 5**
    - Press **Launch Simulation Control**
    - Press **Launch Coordinator**
    - Press **Launch RViZ**
    - Press **PX4-0** (Starts flight controller for Agent 0)
    - Press **LAUNCH AGENT00** (Starts Agent 0 logic)

3.  **Stop System**:
    - Click **Stop All Processes** in the GUI.

## 🎥 Demos & Validation

**Flight Demonstration** - [📹 View Video](media/videos/Demo.mp4)  
*Target acquisition and tracking in the Unreal Engine 5 simulation environment.*

**MATLAB Test** - [📹 View Video](media/videos/MatlabTest.mp4)  
*Target acquisition and tracking in Matlab*

**Camera Gimbal Mechanics** - [📹 Gimbal Movement](media/videos/GimbalMovement.gif)  
*Independent gimbal control test*

## Detailed Usage

### Method 1: Operator Interface (GUI)

The Operator Interface is the central control hub.

```bash
pixi run operator-sim-run      # Simulation Mode
# OR
pixi run operator-hardware-run # Hardware Mode
```

**Features:**
- **Mission Control**: Start/Stop Coordinator, Simulation, Agents, etc.
- **System Logs**: Logs for each process.
- **Real-Time Map**: Real-time 2D map of agents, targets and clusters.
- **Monitoring Feeds**: Live video feeds from agent cameras.

### Method 2: Command Line (Pixi Tasks)

You can run individual components using Pixi tasks defined in `pixi.toml`.

#### Simulation & Core
```bash
/path/to/FlyChamsSim.sh -settings="$FLYCHAMS_ROS2_PATH/config/generated/airsim.json"
pixi run simulation-run        # Launch Simulation Control
pixi run coordinator-sim-run   # Launch Coordinator
```

#### Agents (Docker)
Agents in simulation run inside Docker containers.

```bash
bash tools/docker/build_image.sh        # Build base Docker image
bash tools/build_agent.sh               # Build workspace for agents
bash tools/agent_setup.sh run AGENT00   # Run AGENT00
```

#### PX4 (SITL)
```bash
pixi run simulation-px4-run 0  # Launch PX4 for Agent 0
```

## ⚙️ Configuration

The system uses a workflow where Excel spreadsheets drive the configuration.

1.  **Edit Configuration**: Modify Excel files in `config/` (e.g., `Configuration-TFG.xlsx`).
    - Define mission parameters.
2.  **Generate YAML**: Run the generator to create ROS2 and AirSim config files based on user configuration.
    ```bash
    pixi run generate-settings
    ```
    Files are generated in `config/generated/`.

## 📂 Directory Structure

```
FlyChams-ROS2/
├── config/             # Excel sources & generated YAMLs
├── launch/             # Python launch files
├── src/                # Source code (ROS2 packages)
│   ├── flychams_core/
│   ├── flychams_operator/
│   ├── flychams_coordinator/
│   ├── flychams_agent/
│   └── ...
├── tools/              # Helper scripts
├── pixi.toml           # Pixi environment & task definitions
└── README.md
```

## License

Licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). See [`LICENSE`](LICENSE).

## Contact

Jose Francisco Lopez Ruiz - [josloprui6@alum.us.es](mailto:josloprui6@alum.us.es)
