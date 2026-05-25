# Setup

All commands assume `pwd == FlyChams-ROS2/`.

## Prerequisites

- **Docker** installed and running.
  - **NVIDIA GPU**: install the [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html).
  - **AMD GPU**: ensure Docker has access to `/dev/kfd` and `/dev/dri`.
- **PX4 v1.16** and **Micro-XRCE-DDS Agent v2.4.3** — see [autopilot.md](autopilot.md).

---

## 1. Build Docker Images

Images form a layered hierarchy — build in this order:

```
ros:humble-ros-base
  └── flychams-base
        ├── flychams-coordinator
        ├── flychams-operator
        └── flychams-gpu
              ├── flychams-simulation
              └── flychams-agent
```

```bash
scripts/docker/build_base.sh
scripts/docker/build_gpu.sh
scripts/docker/build_coordinator.sh
scripts/docker/build_simulation.sh
scripts/docker/build_agent.sh
scripts/docker/build_operator.sh
```

### GPU vendor override

Auto-detection picks NVIDIA or AMD. Override with:

```bash
GPU_VENDOR=nvidia scripts/docker/build_gpu.sh
GPU_VENDOR=nvidia scripts/docker/build_simulation.sh
GPU_VENDOR=nvidia scripts/docker/build_agent.sh
```

Valid values: `nvidia`, `amd`, `none`.

---

## 2. Build ROS2 Workspaces

Each script runs `colcon build` inside the corresponding container:

```bash
scripts/build_coordinator_ws.sh
scripts/build_simulation_ws.sh
scripts/build_operator_ws.sh
scripts/build_agent_ws.sh
```

---

## 3. Generate Settings

Reads the configuration spreadsheet via `system.yaml` and generates the mission YAML, AirSim JSON, and Foxglove layout:

```bash
scripts/launch_settings.sh
```

Re-run whenever the spreadsheet or `system.yaml` changes.

| Generated file | Purpose |
|---|---|
| `src/flychams_common/config/generated/mission.yaml` | Mission definition (agents, targets, environment) |
| `src/flychams_common/config/generated/airsim.json` | AirSim vehicle and camera settings |
| `src/flychams_common/config/generated/flychams.json` | Foxglove Studio layout |

---

## 4. Environment Variables

All containers inherit these from the host:

| Variable | Default | Description |
|---|---|---|
| `ROS_DOMAIN_ID` | `0` | ROS2 DDS domain |
| `RMW_IMPLEMENTATION` | `rmw_cyclonedds_cpp` | RMW implementation |
| `CYCLONEDDS_URI` | *(set in containers)* | Path to `cyclonedds.xml` |
| `PX4_AUTOPILOT_PATH` | — | PX4 source tree (see [autopilot.md](autopilot.md)) |
| `Micro_XRCE_DDS_AGENT_PATH` | — | Micro-XRCE-DDS-Agent source tree (see [autopilot.md](autopilot.md)) |
| `GPU_VENDOR` | `auto` | `nvidia` / `amd` / `none` — override GPU auto-detection |
| `FOXGLOVE_PORT` | `8765` | WebSocket port for Foxglove Bridge |

Override inline:

```bash
ROS_DOMAIN_ID=5 scripts/flychams.py sim
```

---

## 5. Network & DDS Tuning

CycloneDDS uses UDP multicast. Apply these host-level tweaks once to avoid dropped messages under high-frequency multi-agent traffic.

### Increase UDP buffer limits

Create `/etc/sysctl.d/99-flychams-net.conf`:

```
net.core.rmem_max=2147483647
net.core.wmem_max=2147483647
net.core.rmem_default=16777216
net.core.wmem_default=16777216
net.ipv4.ipfrag_time=3
net.ipv4.ipfrag_high_thresh=134217728
```

Apply:

```bash
sudo sysctl --system
```

### Enable multicast on loopback

```bash
sudo ip link set lo multicast on
```

To persist across reboots, create a systemd oneshot service:

```bash
sudo tee /etc/systemd/system/multicast-lo.service > /dev/null <<'EOF'
[Unit]
Description=Enable Multicast on Loopback

[Service]
Type=oneshot
ExecStart=/usr/sbin/ip link set lo multicast on

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now multicast-lo.service
```

### CycloneDDS configuration

The project CycloneDDS profile is at `src/flychams_common/config/core/cyclonedds.xml` and is loaded automatically inside every container. No manual action is needed unless you want a custom profile:

```bash
CYCLONEDDS_URI=file:///path/to/custom.xml scripts/flychams.py sim
```

---

Once setup is complete, see [launch.md](launch.md) to start the system.