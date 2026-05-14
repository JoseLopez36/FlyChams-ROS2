# Launch

All scripts are run from the **project root** unless stated otherwise.

## 1. System

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

## 2. Operator

`operator.launch.py` starts the full operator stack inside the `flychams-operator` container:

- **`metrics_node`** — aggregates per-agent, per-target and per-cluster metrics and publishes them as `AgentMetrics`, `TargetMetrics`, `ClusterMetrics` and `MissionMetrics` messages.
- **`markers_node`** — publishes `visualization_msgs/MarkerArray` per agent, target and cluster for 3D visualization in Foxglove.
- **`foxglove_bridge`** — exposes all ROS2 topics over a WebSocket so [Foxglove Studio](https://foxglove.dev/studio) can connect remotely or locally.

### Build

```bash
scripts/build_operator_ws.sh
```

### Launch

```bash
# Interactive (default port 8765)
scripts/launch_operator.sh

# Detached with custom port
DETACH=true FOXGLOVE_PORT=8766 scripts/launch_operator.sh
```

`FOXGLOVE_PORT` is forwarded into the container and picked up by `operator.launch.py` at launch time (default `8765`).

### Connect from Foxglove Studio

Open Foxglove Studio → **Open connection** → **Foxglove WebSocket** → `ws://<host-ip>:8765`.

Import the layout at `foxglove/flychams.json` via **File → Import layout from file**. See [foxglove.md](foxglove.md) for the full interface reference.

---

## 3. Stop

Stop all running FlyChams containers (coordinator, simulation, agents, PX4):

```bash
scripts/stop.sh
```

Containers started with `--rm` are removed automatically on stop.

---

## 4. Logs

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
