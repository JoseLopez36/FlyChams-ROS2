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
| Left | 53.5 % | 3D panel |
| Right | 46.5 % | Tabbed panel with 5 tabs |

---

## Panel Reference

### 3D Situational Awareness

The primary overview panel for real-time situational awareness.

| Setting | Value | Rationale |
|---|---|---|
| Camera distance | 400 m | Covers the full 300 × 300 m operational area |
| Phi (elevation) | 20° | Near top-down with enough 3D cue |
| Theta offset | 315° | NW-looking isometric starting angle |
| Follow mode | `follow-none` | Scene-level overview; no single-agent lock |
| Grid size | 300 m / 30 divisions | 10 m cells |
| Grid draw mode | behind markers | Grid does not occlude visualizations |

---

### Cameras Tab

Three sub-tabs — one per agent — each showing the central view (upper half of the tab area) and the tracking views (2x2 grid in the lower half).

All feeds consume `sensor_msgs/CompressedImage` from:
```
/flychams/operator/<AGENT_ID>/<MULTICAMERA_ID>/image/compressed
```

---

### Agent Metrics Tab

Four time-series plots stacked vertically, all time-synced:

| Panel | Topic path | Unit |
|---|---|---|
| **Agent Speeds** | `/flychams/operator/<AGENT_ID>/metrics.speed` | m/s |
| **Agent Altitudes** | `/flychams/control/<AGENT_ID>/local/position.point.z` | m |
| **Distance to Goal** | `/flychams/operator/<AGENT_ID>/metrics.distance_to_goal` | m |
| **Optimization Duration** | `/flychams/operator/<AGENT_ID>/metrics.optimization_duration` | ms |

All three agents (AGENT00 / AGENT01 / AGENT02) are overlaid on each plot with a legend. Plots share a synchronized timeline (`isSynced: true`).

---

### Mission Tab

Vertically split (50 / 50):

**Top — Mission Overview**

Time-series of system-level scalar metrics from `GlobalMetrics.msg`:

| Series | Field |
|---|---|
| Mission Time | `.mission_time` |
| Agents online | `.total_agents` |
| Active targets | `.total_targets` |
| Active clusters | `.total_clusters` |

Topic: `/flychams/operator/global/metrics`

**Bottom — Agent States**

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

Topic pattern: `/flychams/control/<AGENT_ID>/status`

Field: `.status`

---

### Commands Tab

To be implemented.

---

### Logging Tab

ROS console log viewer:

| Setting | Value |
|---|---|
| Minimum log level | WARN (2) |
| Preload | enabled |
| Hidden nodes | `foxglove_bridge` |