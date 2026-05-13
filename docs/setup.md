# Setup & Launch

All scripts are run from the **project root** unless stated otherwise.

## Prerequisites

1. Docker images built — see [docker.md](docker.md).
2. PX4 simulation image pulled (sim mode only):
   ```bash
   docker pull px4io/px4-dev-nuttx-focal:2021-04-29
   ```
3. `FLYCHAMS_PX4_PATH` set to the absolute path of your PX4 source tree (sim mode only). Add it to your shell profile to persist across sessions:
   ```bash
   export FLYCHAMS_PX4_PATH=/path/to/PX4-Autopilot
   ```

---

## 1. Build Workspaces

Build each ROS2 workspace inside its respective container.

```bash
scripts/build_coordinator_ws.sh
scripts/build_simulation_ws.sh
scripts/build_agent_ws.sh
```

Each script runs `colcon build` for the relevant packages inside the corresponding Docker container.

---

## 2. Generate Settings

Reads `src/flychams_common/config/core/system.yaml` for input/output paths and generates the mission YAML and AirSim JSON files.

```bash
scripts/launch_settings.sh
```

Runs `settings_creator_node` inside the coordinator container. Must be re-run whenever the configuration spreadsheet changes.

---

## 3. Launch

### Unified launcher (recommended)

`scripts/flychams.py` orchestrates all containers for a given mode. Agent IDs are read from `src/flychams_common/config/generated/mission.yaml` by default.

```bash
# Simulation mode — reads all agents from mission.yaml
scripts/flychams.py sim

# Simulation mode — explicit agent subset
scripts/flychams.py sim AGENT00 AGENT01

# Real hardware mode (default)
scripts/flychams.py
```

**Sim mode launch order:**

1. One `PX4-{i}` container per agent via `scripts/launch_px4.sh {i}` (detached).
2. Coordinator container running `coordinator.launch.py`.
3. Simulation container running `simulation.launch.py`.
4. One agent container per agent running `agent.launch.py`.

### Individual launch scripts

For launching a single service independently:

```bash
scripts/launch_px4.sh 0          # PX4 SITL for agent index 0
scripts/launch_coordinator.sh
scripts/launch_simulation.sh
scripts/launch_agent.sh AGENT00
```

Each script sources `install/setup.bash` inside the container before invoking `ros2 launch`. The simulation script additionally sources the AirSim ROS2 overlay at `/home/testuser/FlyChams-Cosys-AirSim/ros2/install/setup.bash`.

---

## 4. Operator

Foxglove Bridge exposes all ROS2 topics over a WebSocket so [Foxglove Studio](https://foxglove.dev/studio) can connect remotely or locally.

### Build

```bash
scripts/docker/build_operator.sh
```

### Launch

```bash
# Interactive (default port 8765)
scripts/launch_operator.sh

# Detached with custom port
DETACH=true FOXGLOVE_PORT=8765 scripts/launch_operator.sh
```

### Connect from Foxglove Studio

Open Foxglove Studio → **Open connection** → **Foxglove WebSocket** → `ws://<host-ip>:8765`.

A layout is provided at `foxglove/flychams.json`. Import it via **File → Import layout from file**. See [foxglove.md](foxglove.md) for a full panel and topic reference.

---

## 5. Stop

Stop all running FlyChams containers (coordinator, simulation, agents, PX4):

```bash
scripts/stop.sh
```

Containers started with `--rm` are removed automatically on stop.

---

## 6. Logs

Tail stdout of all running FlyChams containers simultaneously, each line prefixed with its container name:

```bash
scripts/logs.sh
```

To inspect a single container:

```bash
docker logs -f flychams-coordinator
docker logs -f flychams-simulation
docker logs -f flychams-agent-AGENT00
docker logs -f flychams-px4-0
```

---

## 7. Environment Variables

All containers inherit the following from the host (with defaults):

| Variable | Default | Description |
|---|---|---|
| `ROS_DOMAIN_ID` | `0` | ROS2 DDS domain |
| `FASTDDS_BUILTIN_TRANSPORTS` | `UDPv4` | FastDDS transport |
| `FLYCHAMS_PX4_PATH` | — | Absolute path to the PX4 source tree (required for `launch_px4.sh`) |
| `FOXGLOVE_PORT` | `8765` | WebSocket port for Foxglove Bridge |

Override on the command line:

```bash
ROS_DOMAIN_ID=5 scripts/flychams.py sim
```

---

## 8. Configuration

| File | Purpose |
|---|---|
| `src/flychams_common/config/core/system.yaml` | Paths, simulation settings, GUI settings |
| `src/flychams_common/config/generated/mission.yaml` | Generated mission: agents, targets, environment |
| `src/flychams_common/config/generated/airsim.json` | Generated AirSim settings |