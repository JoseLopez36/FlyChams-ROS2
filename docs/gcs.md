# Ground Control Station (GCS)

The GCS is the command center for FlyChams hardware deployments. It runs on a ground computer and brings up the fleet coordination, operator interface, PX4 bridging, and agents on UAVs via SSH.

---

## GCS Services

The GCS runs two core services locally in Docker containers:

| Service | Container | Purpose |
|---|---|---|
| Operator | `flychams-operator` | Foxglove Studio visualization, metrics, MCAP recording |
| Coordinator | `flychams-coordinator` | Fleet registration, target clustering, agent assignment |

## UAV Services

Each UAV runs two services via Docker, launched remotely via SSH using existing FlyChams scripts:

| Service | Container | Launch Script | Purpose |
|---|---|---|---|
| flychams_agent | `flychams-agent-<ID>` | `launch_agent.sh` | Per-UAV control, tracking, positioning |
| Micro-XRCE-DDS Agent | `flychams-micro-xrce-dds` | `launch_micro_xrce_dds.sh` | Bridges ROS2 DDS to PX4 autopilot (port 8888+idx) |

The GCS executes these scripts on each Jetson via SSH, reusing the same launch tooling used in simulation.

---

## Quick Start

### 1. Prerequisites

- Ground PC with Ubuntu 22.04
- Docker and Docker Compose installed
- [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html) (for GPU-accelerated visualization)
- Network connectivity to UAVs (WiFi/Ethernet)
- SSH key-based authentication to UAVs (Jetson)

### 2. Configure UAV Hosts

Edit `src/flychams_common/config/core/hardware.yaml`:

```yaml
agents:
  AGENT00:
    host: "192.168.1.100"
    user: "jetson"
    workspace: "~/FlyChams-ROS2"
  AGENT01:
    host: "192.168.1.101"
    user: "jetson"
    workspace: "~/FlyChams-ROS2"
```

Each agent in `mission.yaml` needs a matching entry in `hardware.yaml`.

### 3. Setup SSH Keys

The GCS must be able to SSH into each UAV without a password:

```bash
# On GCS, generate SSH key if you don't have one
ssh-keygen -t ed25519 -C "gcs@flychams"

# Copy public key to each UAV
ssh-copy-id jetson@192.168.1.100
ssh-copy-id jetson@192.168.1.101

# Test connection
ssh jetson@192.168.1.100 "echo 'SSH OK'"
```

### 4. Build Images

Build GCS images locally and UAV images on each Jetson:

**On GCS (ground station):**
```bash
scripts/docker/build_base.sh
scripts/docker/build_gpu.sh
scripts/docker/build_coordinator.sh
scripts/docker/build_operator.sh
```

**On each UAV (Jetson):**
```bash
scripts/docker/build_micro_xrce_dds.sh
scripts/docker/build_base.sh
scripts/docker/build_gpu.sh
scripts/docker/build_agent.sh
```

### 5. Generate Mission Settings

```bash
scripts/launch_settings.sh
```

This creates `mission.yaml` with agent definitions used by both GCS and UAVs.

### 6. Launch GCS

```bash
# Bring up all GCS services and launch agents on UAVs via SSH
scripts/flychams.py hw
```

Services start in this order:
1. Operator (Foxglove on port 8765)
2. Coordinator
3. On each UAV via SSH:
   - Micro-XRCE-DDS Agent (port 8888 + agent_idx)
   - flychams_agent

Press **Enter** to stop all services. UAV services are also stopped via SSH.

---

## GCS Modes

### Standard Mode

```bash
scripts/flychams.py hw
```

Runs indefinitely until manually stopped.

### Timed Mission

```bash
# Run for 300 seconds (5 minutes)
scripts/flychams.py hw --duration 300
```

Automatically stops all services after the duration.

### With Recording

```bash
# Record all topics to MCAP
scripts/flychams.py hw --record

# Custom recording name
scripts/flychams.py hw --record --record-name mission_01
```

Recordings save to `recordings/<name>/<name>.mcap`.

---

## Configuration

### Hardware Configuration File

Location: `src/flychams_common/config/core/hardware.yaml`

```yaml
agents:
  <AGENT_ID>:
    host: "<ip_or_hostname>"      # Required: UAV IP address
    user: "<ssh_username>"        # Required:
    workspace: "<path>"           # Required:
```

Example for three UAVs:

```yaml
agents:
  AGENT00:
    host: "192.168.1.100"
    user: "jetson"
    workspace: "~/FlyChams-ROS2"
  AGENT01:
    host: "192.168.1.101"
    user: "jetson"
    workspace: "~/FlyChams-ROS2"
  AGENT02:
    host: "uav-02.local"
    user: "jetson"
```

### Network Requirements

| Requirement | Value |
|---|---|
| ROS Domain ID | Must match on GCS and all UAVs (default: 0) |
| Multicast | Enabled on all interfaces |
| Ports | 7400-7500 (DDS), 8888 (Micro-XRCE-DDS), 8765 (Foxglove), 22 (SSH) |
| SSH | Passwordless key-based auth required |

---

## Troubleshooting

### SSH Connection Fails

1. Test SSH manually:
   ```bash
   ssh jetson@192.168.1.100 "echo 'SSH OK'"
   ```

2. Verify SSH key is installed:
   ```bash
   ssh-copy-id jetson@192.168.1.100
   ```

3. Check agent host configuration in `hardware.yaml`

### Agents Not Connecting

1. Check ROS Domain ID matches:
   ```bash
   # On GCS
   echo $ROS_DOMAIN_ID
   
   # On UAV (via SSH)
   ssh jetson@192.168.1.100 "echo \$ROS_DOMAIN_ID"
   ```

2. Verify DDS discovery:
   ```bash
   # List ROS2 topics on GCS
   docker exec flychams-operator ros2 topic list
   ```

3. Check coordinator logs:
   ```bash
   docker logs flychams-coordinator
   ```

4. Check agent logs on UAV:
   ```bash
   ssh jetson@192.168.1.100 "docker logs flychams-agent-AGENT00"
   ```

### PX4 Not Connecting

1. Verify Micro-XRCE-DDS Agent is running on the UAV:
   ```bash
   ssh jetson@192.168.1.100 "docker ps | grep micro-xrce"
   ```

2. Check PX4 is configured for external DDS (Parameter `XRCE_DDS_AG_IP`):
   - Set to UAV IP address (localhost/127.0.0.1 if PX4 runs on same UAV)
   - Port should be `8888 + agent_idx` (e.g., 8888 for AGENT00, 8889 for AGENT01)

3. View Micro-XRCE-DDS logs on UAV:
   ```bash
   ssh jetson@192.168.1.100 "docker logs flychams-micro-xrce-dds-AGENT00"
   ```