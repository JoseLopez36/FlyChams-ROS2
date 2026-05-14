# Flying Chameleons ROS2: Multi-UAV System for Autonomous Target Tracking

[![ROS2 Humble](https://img.shields.io/badge/ROS2-Humble-blue.svg)](https://docs.ros.org/en/humble/)
[![Unreal Engine 5](https://img.shields.io/badge/Unreal_Engine-5-313131?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![AirSim](https://img.shields.io/badge/AirSim-Enabled-blue)](https://microsoft.github.io/AirSim/)
[![PX4](https://img.shields.io/badge/PX4-Autopilot-black)](https://px4.io/)
[![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc/4.0/)

The Flying Chameleons (FlyChams) project implements a complete system for controlling and coordinating multiple UAVs equipped with modifiable tracking systems. The primary goal is to optimize target tracking through collaborative agent positioning and camera control.

The project leverages:
- **ROS2 Humble** for the distributed robotics framework.
- **Unreal Engine 5** for photorealistic simulation.
- **AirSim** for high-fidelity physics simulation.
- **PX4** for commercial flight control.
- **Docker** for containerised deployment of all system components.

---

<div align="center">
  <img src="media/images/MultiWindowSimulation.png" alt="Full Simulation Environment" width="100%"/>
  <p><em>Complete simulation view showing the UE5 environment, operator GUI, and real-time RViZ data.</em></p>
</div>

|  |  |
| :---: | :---: |
| <img src="media/images/MultiWindowViews.png" alt="Target Tracking" width="70%"/> | <img src="media/images/MultiWindowAgent.png" alt="Drone Close-up" width="100%"/> |
| *Real-time multi-target tracking windows* | *High-fidelity hexacopter model in UE5* |

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

## 🎥 Demos & Validation

**Flight Demonstration** - [📹 View Video](media/videos/Demo.mp4)  
*Target acquisition and tracking in the Unreal Engine 5 simulation environment.*

**MATLAB Test** - [📹 View Video](media/videos/MatlabTest.mp4)  
*Target acquisition and tracking in Matlab.*

**Camera Gimbal Mechanics** - [📹 Gimbal Movement](media/videos/GimbalMovement.gif)  
*Independent gimbal control test.*

## System Architecture

| Package                 | Description                                     |
| ----------------------- | ----------------------------------------------- |
| `flychams_coordinator`  | Perception algorithms for clustering targets and agent assignment |
| `flychams_simulation`   | Simulation framework manager and target control |
| `flychams_agent`        | Agent control, tracking, and positioning       |
| `flychams_operator`     | Metrics aggregation, visualization markers, and Foxglove bridge |
| `flychams_common`       | Core domain models, utilities, and interfaces   |
| `flychams_api`          | Custom messages and services for FlyChams      |

## Prerequisites

### Software Requirements

- **Ubuntu 20.04, 22.04, or 24.04** (or compatible Linux distribution)
- **Docker** - Required for running coordinator, simulation, agent and operator containers. It must be installed with NVIDIA Container Toolkit: [Installing NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)
- **Unreal Engine 5.2.1 (optional)** - Required for creating custom photorealistic simulation environments.

### Hardware Requirements

#### Simulation Mode (PC)
- **CPU**: Intel i7-12700K / AMD Ryzen 7 5800X or better
- **GPU**: NVIDIA RTX 3070 / AMD RX 6800 XT or better (for UE5)
- **RAM**: 16 GB minimum (32 GB recommended)

#### Hardware Mode (Onboard Computer)
- **Device**: NVIDIA Jetson Orin Nano Super
- **OS**: Ubuntu 22.04 (JetPack 6.0+)

## 📦 Installation

```bash
git clone https://github.com/JoseLopez36/FlyChams-ROS2.git
cd FlyChams-ROS2
```

For building Docker images and preparing the workspaces see **[docs/setup.md](docs/setup.md)**.

## 🚀 Quick Start

See **[docs/launch.md](docs/launch.md)** for the full launch reference, including:
- Unified launcher (`scripts/flychams.py`) for sim and hardware modes.
- Individual launch scripts per service.
- Operator / Foxglove bridge.
- Stop and log commands.

## ⚙️ Configuration

The system uses a workflow where Excel spreadsheets drive the configuration.

1.  **Edit Configuration**: Modify Excel files in `config/` (e.g., `Configuration-TFG.xlsx`).
    - Define mission parameters.
2.  **Generate YAML**: Run `scripts/launch_settings.sh` to create ROS2 and AirSim config files.
    Files are generated in `src/flychams_common/config/generated/`.

See `src/flychams_common/config/core/system.yaml` for all configurable paths and parameters.

## 📂 Directory Structure

```
FlyChams-ROS2/
├── docs/               # Documentation (setup, launch, docker, foxglove, simulator)
├── docker/             # Dockerfiles for each container role
├── scripts/            # Build, launch, stop, and log helper scripts
├── src/                # ROS2 packages
│   ├── flychams_common/
│   ├── flychams_coordinator/
│   ├── flychams_agent/
│   ├── flychams_operator/
│   └── flychams_api/
├── foxglove/           # Foxglove Studio layout
└── README.md
```

### Documentation

| File | Contents |
|---|---|
| [docs/setup.md](docs/setup.md) | Prerequisites, building workspaces, generating settings, env vars |
| [docs/launch.md](docs/launch.md) | Launching, stopping, and inspecting logs |
| [docs/docker.md](docs/docker.md) | Building and managing Docker images |
| [docs/simulator.md](docs/simulator.md) | AirSim / UE5 simulator setup |
| [docs/foxglove.md](docs/foxglove.md) | Foxglove Studio operator interface |

## License

Licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). See [`LICENSE`](LICENSE).

## Contact

Jose Francisco Lopez Ruiz - [josloprui6@alum.us.es](mailto:josloprui6@alum.us.es)