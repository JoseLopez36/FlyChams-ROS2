# MATLAB

MATLAB R2026a scripts for trajectory generation and post-flight analysis of MCAP recordings. All scripts live in `matlab/`.

## Prerequisites

- **MATLAB R2026a** with the **ROS Toolbox** (required for `ros2bagreader` and `ros2genmsg`).

---

## 1. Custom Message Generation (one-time setup)

FlyChams uses custom `.msg` types defined in `src/flychams_api/msg/`. MATLAB cannot deserialise these messages from MCAP bags until you generate the corresponding MATLAB stubs.

From within MATLAB, set the current folder to `matlab/` and run:

```matlab
ros2genmsg('../src/')
```

Follow the printed instructions to add the generated folder to the MATLAB path and run `savepath` to make it permanent.

> **Note:** `ros2genmsg` scans every ROS2 package under `src/` and generates stubs for all custom message types it finds (including transitive dependencies). The process may take a few minutes on the first run.

---

## 2. Scripts

### `generate_trajectories.m`

Generates multi-target trajectory sets compatible with the `flychams_simulation` `TrajectoryParser` (tab-separated CSV, columns: `time  x  y  z`, dt = 0.05 s).

**Usage:**

1. Edit the `USER PARAMETERS` section (cluster layout, motion primitives, duration).
2. Run the script.
3. Point the relevant Excel configuration sheet to the new folder name.

Available motion primitives: `lissajous`, `spiral`, `random_walk`, `figure8`, `circle`, `waypoint`. Targets are organised into clusters with configurable centre, radius, and dispersion.

### `load_recording.m`

Loads a FlyChams MCAP recording into a structured MATLAB `rec` struct. Agents, targets, and clusters are discovered automatically from the topic list.

**Usage:**

```matlab
rec = load_recording('../recordings/my_run/my_run_0.mcap', 'Verbose', true);
```

Requires the custom message stubs generated in [Section 1](#1-custom-message-generation-one-time-setup).

### `run_analysis.m`

Master entry point that loads a recording and runs the full analysis suite. Opens a file picker if no path is given.

```matlab
run_analysis % file picker
run_analysis('../recordings/my_run/my_run_0.mcap', 'SaveFigs', true)
```

| Parameter | Default | Description |
|---|---|---|
| `SaveFigs` | `false` | Save PNG of every figure |
| `OutputDir` | `''` | Override auto-generated output directory |
| `Verbose` | `true` | Print progress to console |
| `Decimate` | `10` | Spatial downsampling for trajectory plots |
| `Smooth` | `5` | Time-series smoothing half-window (samples) |

### `plot_metrics.m`

Time-series dashboard for operator metrics. Produces four figures: mission overview, per-agent panel, per-target panel, and per-cluster panel.

```matlab
plot_metrics(rec)
```

### `plot_trajectories.m`

Spatial trajectory visualisation: 2-D top-down view, 3-D view, and per-agent distance-to-goal over time.

```matlab
plot_trajectories(rec)
```

### `plot_tracking.m`

Image-plane tracking quality analysis (crop area fraction, centring error, OOB rate, zoom factor, approximate IoU) per tracking unit.

```matlab
plot_tracking(rec)
```

### `plot_solver.m`

Position-solver and assignment-solver performance analysis (time-series, histogram, CDF, worst-case percentile).

```matlab
plot_solver(rec)
```

---

## 3. Typical Workflow

```matlab
% 1. Generate custom message stubs (once)
ros2genmsg('../src/')

% 2. Load a recording
rec = load_recording('../recordings/my_run/my_run_0.mcap', 'Verbose', true);

% 3. Run individual plots
plot_trajectories(rec);
plot_metrics(rec);
plot_tracking(rec);
plot_solver(rec);

% Or run everything at once with automatic figure export
run_analysis('../recordings/my_run/my_run_0.mcap', 'SaveFigs', true);
```