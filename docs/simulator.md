# Simulator

FlyChams-Sim-UE5 is the Unreal Engine 5 + AirSim environment used for SITL simulation.

## Prerequisites

- FlyChams-Sim-UE5 binary package.
- Generated AirSim settings — run `scripts/launch_settings.sh` (see [setup.md](setup.md)).

---

## NVIDIA Encoder Patch

Consumer NVIDIA GPUs enforce an NVENC session limit. With many simulated cameras (9+) this causes black feeds. Remove the restriction with [nvidia-patch](https://github.com/keylase/nvidia-patch):

```bash
git clone https://github.com/keylase/nvidia-patch.git
cd nvidia-patch
sudo bash patch.sh
```

Re-apply after every driver update.

---

## Launch

Launch the simulator **before** the ROS2 stack.

```bash
scripts/launch_ue5_sim.sh /path/to/FlyChams-Sim-UE5
```

Or manually:

```bash
/path/to/FlyChams-Sim-UE5/FlyChamsSim.sh \
  -settings="<abs-path>/src/flychams_common/config/generated/airsim.json"
```

---

## Optimization Flags

When the display is not needed, run offscreen to free GPU resources:

```bash
OFFSCREEN=true NO_SOUND=true UNATTENDED=true NO_SPLASH=true \
    scripts/launch_ue5_sim.sh /path/to/FlyChams-Sim-UE5
```

| Variable | UE5 flag | Effect |
|---|---|---|
| `OFFSCREEN=true` | `-RenderOffScreen` | No viewport display |
| `NO_SOUND=true` | `-NoSound` | Disable audio engine |
| `UNATTENDED=true` | `-Unattended` | Suppress dialogs |
| `NO_SPLASH=true` | `-nosplash` | Skip splash screen |
| `NO_TEXTURE_STREAMING=true` | `-NoTextureStreaming` | Disable texture streaming |
| `NULL_RHI=true` | `-NullRHI` | Null renderer — **disables all camera feeds** |

> **Warning:** `-NullRHI` disables all GPU rendering including AirSim camera feeds. Do not use it when agents need camera input.