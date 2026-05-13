# Foxglove Operator Interface

Foxglove Studio is used as the operator interface for real-flight and debugging scenarios. The operator Docker container runs `foxglove_bridge`, which exposes all ROS 2 topics over a WebSocket so Foxglove Studio can connect from any machine on the network.

---

## Connection

1. Launch the operator container (see [setup.md](setup.md)):
   ```bash
   scripts/launch_operator.sh
   ```
2. Open [Foxglove Studio](https://foxglove.dev/studio) (desktop or web).
3. **Open connection → Foxglove WebSocket** → `ws://<host-ip>:8765`.
4. Import the layout: **File → Import layout from file** → select `foxglove/flychams.json`.

> The `FOXGLOVE_PORT` environment variable controls the WebSocket port (default `8765`).

---

## Layout Overview

The layout is a two-column split:

| Column | Width | Content |
|---|---|---|
| Left | 58 % | **3D Situational Awareness** panel |
| Right | 42 % | Tabbed panel with 5 tabs |

```
┌──────────────────────────────┬──────────────────────┐
│                              │ CAMERAS │ AGENT METRICS│
│   3D Situational Awareness   │ MISSION │ COMMANDS     │
│         (58 %)               │ LOGGING              │
│                              │      (42 %)          │
└──────────────────────────────┴──────────────────────┘
```

---

## Panel Reference

### 3D Situational Awareness (`3D!scene`)

The primary overview panel for real-time situational awareness.

| Setting | Value | Rationale |
|---|---|---|
| Camera distance | 400 m | Covers the full 300 × 300 m operational area |
| Phi (elevation) | 20° | Near top-down with enough 3D cue |
| Theta offset | 315° | NW-looking isometric starting angle |
| Follow mode | `follow-none` | Scene-level overview; no single-agent lock |
| Grid size | 300 m / 30 divisions | 10 m cells match the mission's horizontal constraint |
| Grid draw mode | behind markers | Grid does not occlude visualizations |

**Pre-subscribed topics:**

| Topic | Content |
|---|---|
| `/flychams/operator/AGENTxx/markers` | Agent poses, trajectories, FOV cones (MarkerArray) |
| `/flychams/operator/TARGET02COUNTxx/markers` | Target true/estimated positions (MarkerArray) |

> Cluster marker topics are not pre-subscribed because cluster IDs are generated at runtime. Add them manually via the topic panel once the mission is running.

**Publish mode** is set to `pose` — right-click anywhere on the scene to publish a `geometry_msgs/PoseStamped` goal to `/move_base_simple/goal`.

---

### CAMERAS tab (`Tab!cams`)

Three sub-tabs — one per agent — each showing:

```
┌─────────────────────────────────────────┐
│                                         │
│           Central Camera                │
│           (MULTICAMERAxx)  (50 %)       │
│                                         │
├──────────────────────┬──────────────────┤
│   Tracking 1         │   Tracking 2     │
├──────────────────────┼──────────────────┤
│   Tracking 3         │   Tracking 4     │
└──────────────────────┴──────────────────┘
```

Camera IDs use a global sequential scheme — 5 cameras per agent:

| Sub-tab | Central | Tracking 1 | Tracking 2 | Tracking 3 | Tracking 4 |
|---|---|---|---|---|---|
| AGENT00 | MULTICAMERA00 (1280 × 720) | MULTICAMERA01 | MULTICAMERA02 | MULTICAMERA03 | MULTICAMERA04 |
| AGENT01 | MULTICAMERA05 (1280 × 720) | MULTICAMERA06 | MULTICAMERA07 | MULTICAMERA08 | MULTICAMERA09 |
| AGENT02 | MULTICAMERA10 (1280 × 720) | MULTICAMERA11 | MULTICAMERA12 | MULTICAMERA13 | MULTICAMERA14 |

Tracking cameras are 960 × 540.

All feeds consume `sensor_msgs/CompressedImage` from:
```
/flychams/operator/<AGENT_ID>/<MULTICAMERA_ID>/image/compressed
```

> The mission config (`src/flychams_common/config/generated/mission.yaml`) must define 5 `multi_cameras` per agent to match this layout. Update the `ids` list and entries for each agent accordingly.

---

### AGENT METRICS tab (`Stack!agmetrics`)

Four time-series plots stacked vertically, all time-synced:

| Panel | Topic path | Unit |
|---|---|---|
| **Agent Speeds** | `/flychams/operator/AGENTxx/metrics.speed` | m/s |
| **Agent Altitudes** | `/flychams/control/AGENTxx/local/position.point.z` | m |
| **Distance to Goal** | `/flychams/operator/AGENTxx/metrics.distance_to_goal` | m |
| **Optimization Duration** | `/flychams/operator/AGENTxx/metrics.optimization_duration` | ms |

All three agents (AGENT00 / AGENT01 / AGENT02) are overlaid on each plot with a legend. Plots share a synchronized timeline (`isSynced: true`).

---

### MISSION tab

Vertically split (50 / 50):

**Top — Mission Overview (`Plot!global`)**

Time-series of system-level scalar metrics from `GlobalMetrics.msg`:

| Series | Field |
|---|---|
| Mission Time | `.mission_time` |
| Agents online | `.total_agents` |
| Active targets | `.total_targets` |
| Active clusters | `.total_clusters` |

Topic: `/flychams/operator/global/metrics`

**Bottom — Agent States (`StateTransitions!states`)**

Discrete state timeline for all three agents sourced from `AgentStatus.msg`:

| Value | State |
|---|---|
| 0 | IDLE |
| 1 | DISARMED |
| 2 | ARMED |
| 3 | TAKING_OFF |
| 4 | TAKEN_OFF |
| 5 | HOVERING |
| 6 | HOVERED |
| 7 | TRACKING |
| 8 | LANDING |
| 9 | LANDED |
| 10 | ERROR |

Topic pattern: `/flychams/control/<AGENT_ID>/status` → field `.status`

---

### COMMANDS tab (`Teleop!ctrl`)

A `geometry_msgs/Twist` velocity joystick configured for aerial control:

| Button | Field | Value |
|---|---|---|
| Up | `linear.z` | +1 (ascend) |
| Down | `linear.z` | −1 (descend) |
| Left | `angular.z` | +1 (yaw CCW) |
| Right | `angular.z` | −1 (yaw CW) |

- **Publish rate:** 5 Hz
- **Auto-stop on release:** enabled
- Set the target topic in Foxglove to the MAVROS setpoint velocity topic for the desired agent: `/mavros/<AGENT_ID>/setpoint_velocity/cmd_vel`

> For waypoint-based commands, prefer right-clicking in the 3D panel (publish mode = `pose`) which directly targets the mission planner.

---

### LOGGING tab (`RosOut!log`)

ROS console log viewer:

| Setting | Value |
|---|---|
| Minimum log level | WARN (2) |
| Preload | enabled |
| Hidden nodes | `foxglove_bridge` (noise suppressed) |

---

## Topic Reference

### Global

| Topic | Type | Description |
|---|---|---|
| `/flychams/bringup/registration` | `flychams_api/Registration` | Agent registration handshake |
| `/flychams/bringup/global_origin` | `geographic_msgs/GeoPointStamped` | Mission geographic origin |
| `/flychams/operator/global/metrics` | `flychams_api/GlobalMetrics` | System-wide counters and mission time |

### Per-Agent (replace `<ID>` with AGENT00 / AGENT01 / AGENT02)

| Topic | Type | Description |
|---|---|---|
| `/flychams/control/<ID>/status` | `flychams_api/AgentStatus` | FSM state (uint8, 0–10) |
| `/flychams/control/<ID>/local/position` | `geometry_msgs/PointStamped` | Local ENU position |
| `/flychams/control/<ID>/global/position` | `geometry_msgs/PointStamped` | Global position |
| `/flychams/coordination/<ID>/setpoint/position` | `geometry_msgs/PointStamped` | Coordinator-assigned position setpoint |
| `/flychams/operator/<ID>/metrics` | `flychams_api/AgentMetrics` | Speed, altitude, distance-to-goal, optimization time |
| `/flychams/operator/<ID>/markers` | `visualization_msgs/MarkerArray` | 3D visualizations (pose, trajectory, FOV) |
| `/flychams/operator/<ID>/<CAM_ID>/image/compressed` | `sensor_msgs/CompressedImage` | Camera feed |
| `/mavros/<ID>/state` | `mavros_msgs/State` | FCU armed / mode / connected |
| `/mavros/<ID>/local_position/odom` | `nav_msgs/Odometry` | Full pose + velocity from MAVROS |

### Per-Target (replace `<ID>` with TARGET02COUNTxx)

| Topic | Type | Description |
|---|---|---|
| `/flychams/targets/<ID>/true_position` | `geometry_msgs/PointStamped` | Ground-truth position |
| `/flychams/perception/<ID>/est_position` | `geometry_msgs/PointStamped` | Estimated position |
| `/flychams/operator/<ID>/metrics` | `flychams_api/TargetMetrics` | Speed, distance traveled |
| `/flychams/operator/<ID>/markers` | `visualization_msgs/MarkerArray` | 3D marker |

### Per-Cluster (replace `<ID>` with runtime cluster ID)

| Topic | Type | Description |
|---|---|---|
| `/flychams/perception/<ID>/assignment` | `flychams_api/ClusterAssignment` | Target membership |
| `/flychams/perception/<ID>/geometry` | `flychams_api/ClusterGeometry` | Bounding geometry |
| `/flychams/operator/<ID>/metrics` | `flychams_api/ClusterMetrics` | Center, radius, speed |
| `/flychams/operator/<ID>/markers` | `visualization_msgs/MarkerArray` | 3D cluster visualization |

---

## Customization

### Adding a new agent

1. Add three `Image!` panel entries in `configById` with the correct `imageTopic` paths.
2. Add a new tab entry to `Tab!cams.tabs`.
3. Add path entries to each of the four metric plots (`Plot!speeds`, `Plot!altitudes`, `Plot!dtgoal`, `Plot!optdur`) and the state transitions panel (`StateTransitions!states`).
4. Add the agent's markers topic to `3D!scene.topics`.

### Following a specific agent in 3D

Change `followMode` in `3D!scene` from `"follow-none"` to `"follow-pose"` and set `followTf` to the agent's TF frame (e.g., `"AGENT00/base_link"`).

### Adjusting the operational grid

Edit `3D!scene.layers.grid`:
- `size` — total grid side length in metres
- `divisions` — number of grid cells per side (`size / divisions` = cell size)

### Persisting layout changes

After editing the layout in Foxglove Studio, export it with **File → Export layout to file** and overwrite `foxglove/flychams.json`.
