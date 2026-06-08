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

Available motion primitives: `lissajous`, `spiral`, `random_walk`, `figure8`, `circle`, `waypoint`. Targets are organised into clusters with configurable centre, radius, and dispersion. Built-in cluster layouts include `default`, `complex-mixed`, `complex-dispersed`, and `complex-concentrated`.

### `analyze_trajectories.m`

Plots agent trajectories, assigned cluster-center trajectories, speed, goal distance, and travel-distance statistics from a FlyChams MCAP recording.

**Usage:**

```matlab
analyze_trajectories('../recordings/my_run/my_run_0.mcap', 20.0, 110.0)
```

The plot uses the same per-agent color palette as the operator visualisation.

### `analyze_tracking.m`

Plots and summarises per-tracking-unit zoom factor and apparent target size from `AgentMetrics`. The central unit is skipped.

```matlab
analyze_tracking('../recordings/my_run/my_run_0.mcap', 20.0, 110.0)
```

### `analyze_fleet.m`

Plots and summarises fleet-level travel distance, assignment swap count, assignment solve duration, mean speed, and goal-distance metrics.

```matlab
analyze_fleet('../recordings/my_run/my_run_0.mcap', 20.0, 110.0)
```

---

## 3. Typical Workflow

```matlab
% 1. Generate custom message stubs (once)
ros2genmsg('../src/')

% 2. Load a recording
bag = '../recordings/my_run/my_run_0.mcap';

% 3. Run individual plots
analyze_trajectories(bag);
analyze_tracking(bag);
analyze_fleet(bag);
```