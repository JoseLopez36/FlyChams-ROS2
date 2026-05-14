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
| Left | 53.5 % | Situational Awareness (top 70 %) + System Log / Fleet Control side-by-side (bottom 30 %) |
| Right | 46.5 % | Tabbed panel (Cameras · Agent Metrics · Mission Metrics) |

The bottom-left area is a horizontal row split:

| Panel | Width |
|---|---|
| System Log | 55 % |
| Fleet Control | 45 % |

---

## Panel Reference

### Situational Awareness

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

### Mission Metrics Tab

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

### System Log

ROS console log viewer:

| Setting | Value |
|---|---|
| Minimum log level | WARN (2) |
| Preload | enabled |
| Hidden nodes | `foxglove_bridge` |

---

### Fleet Control

A tabbed panel placed to the right of System Log in the bottom-left area. Each button is a [**Button extension**](https://github.com/adityakamath/foxglove_extensions/tree/main/button) panel (`Kamath Robotics.button.Button`) that publishes `std_msgs/Bool` on a dedicated topic on press/toggle.

#### Installation

1. Download the latest `.foxe` file from the [releases page](https://github.com/adityakamath/foxglove_extensions/releases) (look for `button-*.foxe`).
2. In Foxglove Studio: **Extensions** (top-right menu) → **Install Extension** → select the `.foxe` file.
3. Restart Foxglove (`Ctrl+R` / `Cmd+R`).
4. The **Button** panel will appear in the panel list.

> Alternatively, build from source: `cd foxglove_extensions/button && npm install && npm run local-install`, then restart Foxglove.

#### Mission tab

Four push buttons in a 2×2 grid. Each publishes `{ data: true }` on press:

| Panel | Topic | Button mode | Label | Purpose |
|---|---|---|---|---|
| Start Mission | `/flychams/fleet/cmd/mission/start` | push | `▶  Start` | Begin the planned mission for all agents |
| Pause Mission | `/flychams/fleet/cmd/mission/pause` | push | `⏸  Pause` | Hold all agents at current position |
| Abort Mission | `/flychams/fleet/cmd/mission/abort` | push | `⏹  Abort` | Abort mission and hold position |

#### Arm / Disarm tab

Two panels side-by-side:

| Panel | Topic | Button mode | Active / Inactive label | Purpose |
|---|---|---|---|---|
| Arm / Disarm All | `/flychams/fleet/cmd/arm` | toggle | `🔓  Armed` / `🔒  Disarmed` | Publishes `true` on arm, `false` on disarm |
| Return to Home | `/flychams/fleet/cmd/mission/rth` | push | `🏠  Home` | Command all agents to return to home |