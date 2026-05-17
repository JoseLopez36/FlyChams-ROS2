# Docker

FlyChams services run in Docker containers built from a layered image hierarchy.

## Image Hierarchy

```
ros:humble-ros-base
  └── flychams-base              (docker/base.Dockerfile)
        ├── flychams-coordinator (docker/coordinator.Dockerfile)
        ├── flychams-simulation  (docker/simulation.Dockerfile)
        ├── flychams-agent       (docker/agent.Dockerfile)
        └── flychams-operator    (docker/operator.Dockerfile)
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

### flychams-simulation

Extends `flychams-base` with no additional layers.

### flychams-agent

Extends `flychams-base` with agent-specific tooling.

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

- Docker with NVIDIA Container Toolkit: [Installing NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html).

### Build

Build images in dependency order (base must be built first).

```bash
scripts/docker/build_base.sh         # flychams-base
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
- `--runtime nvidia --gpus all` for NVIDIA GPU access.
- Project root mounted at `/home/testuser/FlyChams-ROS2`.
- `ROS_DOMAIN_ID` and `FASTDDS_BUILTIN_TRANSPORTS` forwarded from the host.

The agent container additionally receives `AGENT_ID` as an environment variable. The operator container does not require GPU access and omits `--runtime nvidia`.

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