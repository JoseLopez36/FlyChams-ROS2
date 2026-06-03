# Performance

Measured hardware utilisation for the FlyChams system under the two streaming configurations
(Multi-Camera and Multi-Window) with 1, 2, and 3 agents.

All values are reported as **mean** and **max** over a full mission run.
CPU and GPU figures are in **percent (%)**; RAM and VRAM figures are in **GB**.

---

## Test platform

| Component | Specification |
|---|---|
| CPU | Intel Core i7-13620H |
| GPU | NVIDIA GeForce RTX 5070 Laptop (8 GB VRAM) |
| RAM | 32 GB |
| OS | Ubuntu 22.04 |

---

## Streaming configurations

| Configuration | Description |
|---|---|
| **Multi-Camera** | One 720p central view + two 540p tracking views per agent |
| **Multi-Window** | One 4k resolution camera stream per agent |

---

## Results

### Multi-Camera

| Metric | 1 agent (mean / max) | 2 agents (mean / max) | 3 agents (mean / max) |
|---|---|---|---|
| CPU (%) | 23.83 / 53.50 | 44.30 / 94.23 | 64.05 / 91.77 |
| GPU (%) | 56.01 / 73.00 | 67.74 / 87.00 | 58.03 / 94.00 |
| RAM (GB) | 6.34 / 6.47 | 7.82 / 8.40 | 8.57 / 9.25 |
| VRAM (GB) | 3.23 / 3.24 | 4.91 / 5.32 | 6.48 / 7.16 |

### Multi-Window

| Metric | 1 agent (mean / max) | 2 agents (mean / max) | 3 agents (mean / max) |
|---|---|---|---|
| CPU (%) | 29.23 / 56.79 | 53.05 / 86.27 | 71.69 / 95.18 |
| GPU (%) | 51.34 / 63.00 | 75.31 / 92.00 | 66.72 / 96.00 |
| RAM (GB) | 6.32 / 6.46 | 8.57 / 8.68 | 9.40 / 9.57 |
| VRAM (GB) | 3.40 / 3.41 | 5.09 / 5.18 | 6.59 / 6.60 |

---

## Notes

- CPU usage scales near-linearly with the number of agents; it approaches saturation at 3 agents under
  both configurations.
- GPU usage is dominated by the AirSim rendering workload and the video encoding pipeline; individual
  readings vary with scene complexity.
- VRAM consumption grows approximately linearly per additional agent stream and remains within the
  8 GB budget at 3 agents.
- RAM usage stays well within the 32 GB system budget across all tested configurations.
- Performance metrics are collected by `metrics_creator_node` via `GPU_VENDOR=nvidia` and reported on
  the `/flychams/operator/mission/metrics` topic.