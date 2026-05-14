# Setup

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
scripts/build_operator_ws.sh
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

## 3. Environment Variables

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

## 4. Configuration

| File | Purpose |
|---|---|
| `src/flychams_common/config/core/system.yaml` | Paths, simulation settings, GUI settings |
| `src/flychams_common/config/generated/mission.yaml` | Generated mission: agents, targets, environment |
| `src/flychams_common/config/generated/airsim.json` | Generated AirSim settings |

---

Once setup is complete, see [launch.md](launch.md) to start the system.