# Jetson Orin Nano Setup

This guide covers setting up FlyChams-ROS2 on the **NVIDIA Jetson Orin Nano** for running `flychams_agent` on the UAV.

---

## Prerequisites

### Hardware

- **NVIDIA Jetson Orin Nano** (8GB or 16GB)
- **JetPack 5.1 or later** installed
- **Ubuntu 22.04** (comes with JetPack)

### JetPack Installation

If not already installed, flash JetPack using [NVIDIA SDK Manager](https://developer.nvidia.com/nvidia-sdk-manager):

1. Download and install SDK Manager on a host PC
2. Connect the Jetson via USB-C (recovery mode)
3. Select JetPack 5.1+ and flash

Verify installation:

```bash
# Check JetPack version
cat /etc/nv_tegra_release

# Expected output includes "R35" (JetPack 5.x) or "R36" (JetPack 6.x)
```

---

## Docker Setup

### 1. Enable NVIDIA Container Runtime

JetPack includes the NVIDIA container runtime. Enable it as the default:

```bash
sudo apt install -y jq
sudo jq '. + {"default-runtime": "nvidia"}' /etc/docker/daemon.json | \
    sudo tee /etc/docker/daemon.json.tmp
sudo mv /etc/docker/daemon.json.tmp /etc/docker/daemon.json
sudo systemctl restart docker
```

Add your user to the `docker` group:

```bash
sudo usermod -aG docker $USER
newgrp docker
```

More info on [NVIDIA Docker Setup](https://docs.nvidia.com/jetson/orin-nano-devkit/user-guide/latest/setup_docker.html).

### 2. Verify Tegra Devices

Ensure the Jetson V4L2 video decoder/encoder devices are accessible:

```bash
ls -la /dev/nvhost-*
```

Expected output:
```
/dev/nvhost-ctrl
/dev/nvhost-ctrl-gpu
/dev/nvhost-nvdec   # H.265 decoder
/dev/nvhost-nvenc   # H.265 encoder
/dev/nvhost-vic     # Video Image Compositor
```

If devices are missing, check that the `nvidia` kernel modules are loaded:

```bash
lsmod | grep nvhost
```

---

## Build Docker Images

On the Jetson, GPU vendor is auto-detected as `jetson`. Build the images:

```bash
# Build base and GPU layers
scripts/docker/build_base.sh
scripts/docker/build_gpu.sh

# Build agent image
scripts/docker/build_agent.sh
```

To force Jetson detection explicitly:

```bash
GPU_VENDOR=jetson scripts/docker/build_gpu.sh
GPU_VENDOR=jetson scripts/docker/build_agent.sh
```

### Micro-XRCE-DDS Agent for Hardware Mode

When using the [GCS hardware mode](gcs.md), the Jetson must also run the Micro-XRCE-DDS Agent to bridge PX4 autopilot to ROS2. This runs as a separate Docker container. Build the image on the Jetson following `autopilot.md` — Micro-XRCE-DDS Agent.

The Micro-XRCE-DDS Agent runs on port `8888 + agent_idx` (e.g., 8888 for AGENT00, 8889 for AGENT01).

---

## Run flychams_agent on Jetson

The `run_agent.sh` script auto-detects Jetson hardware and configures:
- `--runtime nvidia` (required for Tegra, different from desktop `--gpus all`)
- Tegra device mounts (`/dev/nvhost-*`)
- `video` group access for V4L2
- Bind-mounts for Tegra libraries and NVIDIA GStreamer plugins (`libgstnv*.so`) from the Jetson host

Install the host GStreamer stack once on the Jetson (outside Docker):

```bash
sudo apt-get update
sudo apt-get install -y nvidia-l4t-gstreamer
```

### Start an Agent

```bash
# Run interactively
scripts/docker/run_agent.sh AGENT00

# Or run detached (background)
DETACH=true scripts/docker/run_agent.sh AGENT00
```

### Verify Hardware Decoding

Inside the container, check that GStreamer can use the Jetson decoder:

```bash
# Enter running container
docker exec -it flychams-agent-AGENT00 bash

# Test decoder availability
gst-inspect-1.0 nvv4l2decoder

# Test a sample decode pipeline (should show capabilities)
gst-launch-1.0 fakesrc ! h265parse ! nvv4l2decoder ! fakesink
```

### Expected GStreamer Pipeline

On Jetson, `flychams_agent` uses the following hardware-accelerated pipeline:

```
rtspsrc location=<rtsp_url> latency=0 protocols=tcp
    ! rtph265depay
    ! h265parse
    ! nvv4l2decoder enable-max-performance=1 drop-frame-interval=1
    ! nvvidconv
    ! video/x-raw,format=BGRx
    ! videoconvert
    ! video/x-raw,format=BGR
    ! appsink drop=true max-buffers=1 sync=false
```

Key elements:
- `nvv4l2decoder` — Hardware H.265 decoder using Tegra V4L2 interface
- `nvvidconv` — GPU-accelerated color space conversion
- `BGRx` → `BGR` — Convert GPU-native format to OpenCV-compatible

---

## Troubleshooting

### Container fails to start

Check Docker runtime configuration:

```bash
cat /etc/docker/daemon.json
# Should have "default-runtime": "nvidia"

# Check runtime is available
docker info | grep -i nvidia
```

### Video decoding fails

Enable GStreamer debug logging inside the container:

```bash
GST_DEBUG=3 ros2 launch flychams_agent agent.launch.py
```

Common errors:

| Error | Cause | Solution |
|---|---|---|
| `nvv4l2decoder not found` | Host GStreamer plugins missing | Install `nvidia-l4t-gstreamer` on the Jetson host, then restart the agent container |
| `Failed to open device` | Tegra devices not accessible | Check `/dev/nvhost-nvdec` exists, verify Docker device mounts |
| `Could not negotiate format` | Color space mismatch | Verify `nvvidconv` and `videoconvert` are in pipeline |

### Check hardware acceleration is active

Monitor GPU usage during streaming:

```bash
# On Jetson host (outside container)
sudo tegrastats
```

Look for `GR3D` (GPU) usage during agent streaming. If `GR3D` is 0%, software fallback may be active.

### High CPU usage

If CPU usage is high (>50% per stream), the hardware decoder may not be engaged:

1. Verify `GPU_VENDOR=jetson` inside container:
   ```bash
   docker exec flychams-agent-AGENT00 printenv | grep GPU_VENDOR
   ```

2. Check GStreamer is using `nvv4l2decoder` not `avdec_h265`:
   ```bash
   # In container, check the actual pipeline
   grep "nvv4l2decoder" /home/testuser/FlyChams-ROS2/src/flychams_agent/src/stream/agent_stream.cpp
   ```

---

## Performance Tips

### Optimize Decoder Settings

The `nvv4l2decoder` is configured for maximum performance:
- `enable-max-performance=1` — Disables power saving for decoder.
- `drop-frame-interval=1` — Allows frame dropping if decoder is overwhelmed.

### Jetson Power Mode

Set maximum performance mode for consistent streaming:

```bash
sudo nvpmodel -m 0   # 0 = MAXN (maximum performance)
sudo jetson_clocks   # Lock clocks at maximum
```

### Memory Optimization

Jetson Orin Nano has limited RAM. Monitor usage:

```bash
# On host
free -h
tegrastats
```

If memory is constrained:
- Reduce `central_view` and `tracking_view` resolutions in `nodes.yaml`.
- Reduce `stream_delay_ms` to stagger stream startup.