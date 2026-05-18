# Performance Recommendations

Tuning guidelines to improve throughput and latency in the FlyChams stack.

---

## 1. Increase OS UDP Buffer Limits

ROS2 DDS (CycloneDDS) relies on UDP sockets. The default Linux kernel buffer sizes are often too small for high-frequency, multi-agent traffic and will cause silently dropped messages.

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

## 3. Enable Multicast on Loopback Interface

CycloneDDS uses multicast for discovery. The loopback interface (`lo`) must have multicast enabled.

### Temporary (until reboot)

```bash
sudo ip link set lo multicast on
```

### Permanent (systemd service)

Create the service file:

```bash
sudo nano /etc/systemd/system/multicast-lo.service
```

Paste the following:

```
[Unit]
Description=Enable Multicast on Loopback

[Service]
Type=oneshot
ExecStart=/usr/sbin/ip link set lo multicast on

[Install]
WantedBy=multi-user.target
```

Enable and start the service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable multicast-lo.service
sudo systemctl start multicast-lo.service
```

### Verify

```bash
ip link show lo
```

The output should include `MULTICAST`:

```
1: lo: <LOOPBACK,MULTICAST,UP,LOWER_UP> mtu 65536 ...
```

---

## 4. CycloneDDS XML Configuration

The FlyChams CycloneDDS profile is located at:

```
src/flychams_common/config/core/cyclonedds.xml
```

It configures:

| Setting | Purpose |
|---|---|
| `lo` interface | Loopback for local communication |
| `MaxMessageSize` | 65500B for large messages |
| `ParticipantIndex=none` | Avoids participant index limits on ROS 2 Jazzy |
| `SocketReceiveBufferSize` | 10MB minimum buffer |

### Activate

Set the RMW implementation and point CycloneDDS to the profile before launching:

```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export CYCLONEDDS_URI=file:///home/testuser/FlyChams-ROS2/src/flychams_common/config/core/cyclonedds.xml
```

The project root is mounted at `/home/testuser/FlyChams-ROS2` inside every container, so the path is fixed.

All `docker/run_*.sh` scripts forward these variables into the container automatically. To override, pass different values before calling `flychams.py` or any individual launch script:

```bash
RMW_IMPLEMENTATION=rmw_cyclonedds_cpp \
CYCLONEDDS_URI=file:///path/to/custom_cyclonedds.xml \
    scripts/flychams.py sim
```

Or export them once in your shell session before running any launch scripts.

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