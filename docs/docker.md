# Docker

FlyChams services run in Docker containers built from a layered image hierarchy.

## Image Hierarchy

```
ros:humble-ros-base
  └── flychams-base              (docker/base.Dockerfile)
        ├── flychams-coordinator (docker/coordinator.Dockerfile)
        ├── flychams-operator    (docker/operator.Dockerfile)
        └── flychams-gpu         (docker/gpu.Dockerfile)
              ├── flychams-simulation  (docker/simulation.Dockerfile)
              └── flychams-agent       (docker/agent.Dockerfile)
```

## Images

### flychams-base

Common foundation for all FlyChams services.

- **Base**: `ros:humble-ros-base`
- **ROS2 packages**: `tf2-sensor-msgs`, `tf2-geometry-msgs`, `vision-opencv`, `geographic-msgs`, `image-transport`, `pcl-ros`, `pcl-conversions`
- **System**: `libyaml-cpp-dev`
- **User**: Non-root `testuser` with passwordless sudo, ROS2 environment sourced in `.bashrc`

### flychams-coordinator

Extends `flychams-base` with no additional layers.

### flychams-gpu

Extends `flychams-base` with GStreamer and GPU-accelerated video decode. Shared by `flychams-simulation` and `flychams-agent`.

- **GStreamer**: `gstreamer1.0-tools`, plugins-base/good/bad/ugly, `libav`, dev headers
- **NVIDIA**: `gstreamer1.0-plugins-bad` (nvh265dec)
- **AMD**: `gstreamer1.0-vaapi`, `libva-dev`, `libva-drm2`, `libva-x11-2`, `mesa-va-drivers`

### flychams-simulation

Extends `flychams-gpu` with AirSim ROS2 integration and simulation view streaming.

- **FlyChams-Cosys-AirSim**: Cloned from `flychams` branch, AirSim C++ libs built, `airsim_interfaces` and `airsim_wrapper` built into `/home/testuser/FlyChams-Cosys-AirSim/ros2`.
- **simulation_stream_node**: Bridges AirSim RTSP streams (`SCENARIOCAM`, `AGENTCAM_<id>`, `PAYLOADCAM_<id>`) to ROS2 image topics via GStreamer.

### flychams-agent

Extends `flychams-gpu` with agent-specific tooling.

- **px4_msgs**: Built from source (`release/1.16` branch) into `/home/testuser/px4_msgs_ws` — provides PX4 uORB ROS2 message types.
- **Ultralytics**: YOLO models via `ultralytics` + `lapx`.

### flychams-operator

Extends `flychams-base` with Foxglove Bridge for remote monitoring via Foxglove Studio.

- **Foxglove Bridge**: `ros-humble-foxglove-bridge` — exposes all ROS2 topics over a WebSocket on port `8765`.

### px4-dev-nuttx-focal (external)

Required for PX4 SITL simulation. Not built locally — pulled from Docker Hub.

- **Image**: `px4io/px4-dev-nuttx-focal:2022-08-12`
- **Purpose**: Provides the NuttX toolchain used to compile and run PX4 v1.16 firmware in software-in-the-loop (SITL) mode.

See [autopilot.md](autopilot.md) for installation and build instructions.

### micro-xrce-dds-agent (external)

Required for ROS2 communication with PX4 via the Micro-XRCE-DDS protocol.

- **Image**: `micro-xrce-dds-agent` (built from the `Dockerfile` at the root of the Micro-XRCE-DDS-Agent v2.4.3 source tree)
- **Purpose**: Bridges PX4 uORB topics to ROS2 DDS, enabling direct topic communication between PX4 SITL and the FlyChams ROS2 stack.

See [autopilot.md](autopilot.md) for installation and build instructions.

## Scripts

All scripts live in `scripts/` and are run from the project root.

### Prerequisites

#### NVIDIA GPU
- Docker with NVIDIA Container Toolkit: [Installing NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html).

#### AMD GPU
- Docker with device access to `/dev/kfd` (AMD GPU kernel driver) and `/dev/dri` (Direct Rendering Interface).
- No additional container toolkit required for VAAPI hardware decoding.
- For PyTorch ROCm support, install ROCm on the host.

### Build

Build images in dependency order (base must be built first).

```bash
scripts/docker/build_base.sh         # flychams-base
scripts/docker/build_gpu.sh          # flychams-gpu
scripts/docker/build_coordinator.sh  # flychams-coordinator
scripts/docker/build_simulation.sh   # flychams-simulation
scripts/docker/build_agent.sh        # flychams-agent
scripts/docker/build_operator.sh     # flychams-operator
```

### Run

Start containers in interactive mode. Existing containers with the same name are removed first.

```bash
scripts/docker/run_coordinator.sh
scripts/docker/run_simulation.sh
scripts/docker/run_agent.sh AGENT00
scripts/docker/run_operator.sh
```

All containers use:
- `--network host` for ROS2 DDS discovery.
- GPU access via vendor-specific Docker flags (auto-detected):
  - **NVIDIA**: `--runtime nvidia --gpus all -e NVIDIA_DRIVER_CAPABILITIES=all -e NVIDIA_VISIBLE_DEVICES=all`
  - **AMD**: `--device /dev/kfd --device /dev/dri --group-add video --group-add render -e ROCR_VISIBLE_DEVICES=all`
  - **Intel**: `--device /dev/dri --group-add video -e LIBVA_DRIVER_NAME=iHD`
- Project root mounted at `/home/testuser/FlyChams-ROS2`.
- `ROS_DOMAIN_ID`, `RMW_IMPLEMENTATION`, and `CYCLONEDDS_URI` forwarded from the host.

The agent container additionally receives `AGENT_ID` as an environment variable. The simulation and agent containers both receive `HW_VENDOR` for GStreamer hardware decode selection. The operator container does not require GPU access.

### GPU Vendor Selection

Override auto-detection by setting the `GPU_VENDOR` environment variable:

```bash
GPU_VENDOR=nvidia scripts/docker/run_agent.sh AGENT00
GPU_VENDOR=amd scripts/docker/run_agent.sh AGENT00
GPU_VENDOR=none scripts/docker/run_agent.sh AGENT00   # CPU only
```

The `GPU_VENDOR` build arg is also respected by `build_gpu.sh`, `build_simulation.sh`, and `build_agent.sh`:

```bash
GPU_VENDOR=amd scripts/docker/build_gpu.sh
GPU_VENDOR=amd scripts/docker/build_simulation.sh
GPU_VENDOR=amd scripts/docker/build_agent.sh
```

### Exec

Open an interactive shell or run a command in a running container.

```bash
scripts/docker/exec_coordinator.sh
scripts/docker/exec_simulation.sh
scripts/docker/exec_agent.sh AGENT00
```

### CMD Override

All run and exec scripts accept a `CMD` environment variable to override the default command.

**Run**:
```bash
CMD="ros2 topic list" scripts/docker/run_coordinator.sh
```

**Exec**:
```bash
CMD="ros2 topic list" scripts/docker/exec_coordinator.sh
```