# Performance Recommendations

Tuning guidelines to improve throughput and latency in the FlyChams stack.

---

## 1. Increase OS UDP Buffer Limits

ROS2 DDS (FastDDS) relies on UDP sockets. The default Linux kernel buffer sizes are often too small for high-frequency, multi-agent traffic and will cause silently dropped messages.

### Apply permanently (recommended)

Add the following lines to `/etc/sysctl.d/99-flychams-net.conf` (create the file):

```
# Maximum socket receive/send buffer size (2 GiB)
net.core.rmem_max=2147483647
net.core.wmem_max=2147483647

# Default socket receive/send buffer size (16 MB)
net.core.rmem_default=16777216
net.core.wmem_default=16777216

# IP fragmentation — critical for large DDS messages (point clouds, images)
net.ipv4.ipfrag_time=3
net.ipv4.ipfrag_high_thresh=134217728
```

Reload without rebooting:

```bash
sudo sysctl --system
```

### Verify

```bash
sysctl net.core.rmem_max net.core.wmem_max net.ipv4.ipfrag_time net.ipv4.ipfrag_high_thresh
```

> **Note:** These settings apply to the host. Containers running with `--network host` inherit them automatically.

---

## 2. Hardware Acceleration in Foxglove Studio

Foxglove Studio performs all panel rendering in WebGL. Without hardware acceleration the GPU is bypassed, causing high CPU usage and degraded frame rates on the Cameras and 3D panels.

### Desktop app (Electron)

The desktop app should automatically use hardware acceleration. If you're experiencing performance issues, ensure that your GPU drivers are up to date and that hardware acceleration is not being disabled by your system settings.

### Web app (browser)

- Use a Chromium-based browser (Chrome, Edge) — they expose the most complete WebGL implementation.
- Navigate to `chrome://gpu` and confirm **WebGL** and **Hardware accelerated** are listed under *Graphics Feature Status*.
- Disable browser extensions that intercept rendering (e.g. some VPN or screen-capture extensions can downgrade the compositor).

---

## 3. Shared Memory Transport in FastDDS

When all communicating ROS2 nodes run on the **same host** (e.g. all containers with `--network host`), FastDDS Shared Memory Transport (SHM) eliminates the UDP copy overhead and significantly reduces latency and CPU usage for large messages such as compressed images.

### Enable via environment variable

```bash
export FASTDDS_BUILTIN_TRANSPORTS=SHM
```

Pass it to the containers before launching:

```bash
FASTDDS_BUILTIN_TRANSPORTS=SHM scripts/flychams.py sim
```

Or set it in the shell that sources the containers so all `docker run` calls inherit it (see [setup.md](setup.md) — *Environment Variables*).

> **Note:** SHM requires all nodes to share the same Linux kernel. All `docker/run_*.sh` scripts already set `--network host` and `--ipc=host`, which gives every container access to the same `/dev/shm` namespace — required for SHM to work across containers. For fine-grained control over buffer sizes use the FastDDS XML profile described in section 4 below.

---

## 4. FastDDS XML Profile (SHM + UDP)

The FlyChams FastDDS profile is located at:

```
src/flychams_common/config/core/fastdds.xml
```

It configures two transports:

| Transport | Purpose |
|---|---|
| `shm_transport` | Shared memory — zero-copy for all intra-host traffic (compressed images, metrics) |
| `udp_transport` | UDPv4 — used for PX4/Micro-XRCE-DDS agent communication and cross-host discovery |

### Activate

Point FastDDS to the profile before launching. The project root is mounted at `/home/testuser/FlyChams-ROS2` inside every container, so the path is fixed:

```bash
export FASTRTPS_DEFAULT_PROFILES_FILE=/home/testuser/FlyChams-ROS2/src/flychams_common/config/core/fastdds.xml
```

All `docker/run_*.sh` scripts default to this path and forward the variable into the container automatically. To override, pass a different path before calling `flychams.py` or any individual launch script:

```bash
FASTRTPS_DEFAULT_PROFILES_FILE=/path/to/custom_fastdds.xml \
    scripts/flychams.py sim
```

Or export it once in your shell session before running any launch scripts.

> **Note:** When `FASTRTPS_DEFAULT_PROFILES_FILE` is set, FastDDS ignores `FASTDDS_BUILTIN_TRANSPORTS`. The two variables are mutually exclusive — set one or the other, not both.

---

## 5. UE5 Simulation Offscreen Rendering & Optimization Flags

Rendering the Unreal Engine 5 viewport consumes significant GPU and CPU resources. When the simulation display is not needed, run the simulator offscreen and apply the flags below to maximize rendering throughput for the AirSim cameras.

### Offscreen rendering

```bash
/path/to/FlyChams-Sim-UE5/FlyChamsSim.sh \
  -settings="<abs-path-to-airsim.json>" \
  -RenderOffScreen
```

`-RenderOffScreen` disables the display output entirely. The AirSim camera pipeline continues to render internally, which is what the agent containers consume.

### Recommended optimization flags

Append any combination of the following to the launch command:

| Flag | Effect |
|---|---|
| `-RenderOffScreen` | Disables viewport display; reduces GPU load when visual output is not needed |
| `-NoSound` | Disables the audio engine |
| `-Unattended` | Suppresses all dialog boxes and crash reporters |
| `-NullRHI` | Uses the null rendering backend — **only** use this if AirSim cameras are not required (no visual output at all) |
| `-nosplash` | Skips the UE splash screen on startup |
| `-NoTextureStreaming` | Disables texture streaming (useful when GPU VRAM is the bottleneck) |
| `r.SetRes 640x480` | Reduces the back-buffer resolution when the viewport is still shown |

### Full example (offscreen, no sound, unattended)

```bash
OFFSCREEN=true NO_SOUND=true UNATTENDED=true NO_SPLASH=true \
    scripts/launch_ue5_sim.sh /path/to/FlyChams-Sim-UE5
```

Each flag is controlled by an environment variable (all default to `false`):

| Variable | UE5 flag injected |
|---|---|
| `OFFSCREEN=true` | `-RenderOffScreen` |
| `NO_SOUND=true` | `-NoSound` |
| `UNATTENDED=true` | `-Unattended` |
| `NO_SPLASH=true` | `-nosplash` |
| `NO_TEXTURE_STREAMING=true` | `-NoTextureStreaming` |
| `NULL_RHI=true` | `-NullRHI` |

Any additional arguments passed after the UE5 path are forwarded directly to the binary.

> **Note:** `-NullRHI` disables all GPU rendering including the AirSim camera feeds. Do **not** use it when agents need camera input.

See [simulator.md](simulator.md) for the standard launch instructions and the AirSim settings file reference.