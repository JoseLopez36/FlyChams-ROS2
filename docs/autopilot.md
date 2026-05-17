# Autopilot

FlyChams uses **PX4 v1.16** as the flight controller firmware in SITL mode, and the **Micro-XRCE-DDS Agent v2.4.3** as the middleware bridge between PX4 uORB topics and ROS2.

---

## Environment Variables

The following host environment variables are required for autopilot operation:

| Variable | Description |
|---|---|
| `PX4_AUTOPILOT_PATH` | Absolute path to the PX4-Autopilot source tree |
| `Micro_XRCE_DDS_AGENT_PATH` | Absolute path to the Micro-XRCE-DDS-Agent source tree |

Add them to your shell profile to persist across sessions:

```bash
export PX4_AUTOPILOT_PATH=/path/to/PX4-Autopilot
export Micro_XRCE_DDS_AGENT_PATH=/path/to/Micro-XRCE-DDS-Agent
```

---

## PX4 Installation

> Reference: [PX4 v1.16 Documentation](https://docs.px4.io/v1.16/en/)

### 1. Clone PX4 v1.16

```bash
git clone --branch v1.16.0 --recursive https://github.com/PX4/PX4-Autopilot.git
```

Set `PX4_AUTOPILOT_PATH` to the cloned directory.

### 2. Pull the PX4 Docker image

PX4 SITL is compiled and run inside the official NuttX toolchain container:

```bash
docker pull px4io/px4-dev-nuttx-focal:2022-08-12
```

### 3. Build PX4 SITL

Use the provided `docker_run.sh` helper script inside the PX4 source tree to build with the correct toolchain:

```bash
cd $PX4_AUTOPILOT_PATH
./Tools/docker_run.sh "make px4_sitl_default none_iris"
```

This compiles the SITL firmware targeting the Iris quadrotor model. Once complete, PX4 is ready to be launched by the FlyChams scripts.

---

## Micro-XRCE-DDS Agent Installation

The Micro-XRCE-DDS Agent bridges PX4 uORB messages to ROS2 DDS, enabling direct topic communication between PX4 and the FlyChams ROS2 stack.

> Version used: **Micro-XRCE-DDS Agent v2.4.3**

### 1. Clone Micro-XRCE-DDS Agent v2.4.3

```bash
git clone --branch v2.4.3 --recursive https://github.com/eProsima/Micro-XRCE-DDS-Agent.git
```

Set `Micro_XRCE_DDS_AGENT_PATH` to the cloned directory.

### 2. Build the Docker image

A `Dockerfile` is provided at the root of the Micro-XRCE-DDS-Agent repository:

```bash
cd $Micro_XRCE_DDS_AGENT_PATH
docker build -t micro-xrce-dds-agent .
```

### 3. Run the agent

The agent is launched by the FlyChams scripts automatically in simulation mode. To run it manually:

```bash
docker run --rm --network host micro-xrce-dds-agent udp4 -p 8888
```

---

## Usage

Once PX4 is built and the Micro-XRCE-DDS Agent image exists, the FlyChams launch scripts handle autopilot startup automatically. See [launch.md](launch.md) for the full launch reference.

- `scripts/launch_px4.sh {i}` — starts a `PX4-{i}` container running PX4 SITL for agent index `i`.
- The Micro-XRCE-DDS Agent is started alongside PX4 to expose uORB topics on the ROS2 network.