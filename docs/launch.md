# Launch

All commands assume `pwd == FlyChams-ROS2/`. Complete [setup.md](setup.md) first.

---

## 1. Start

### Unified launcher (recommended)

```bash
scripts/flychams.py sim                  # simulation — all agents from mission.yaml
scripts/flychams.py sim AGENT00 AGENT01  # simulation — explicit subset
scripts/flychams.py                      # real hardware
```

Sim mode starts containers in this order: PX4 instances → Micro-XRCE-DDS → Coordinator → Simulation → Agents.

### With recording

```bash
scripts/flychams.py sim --record                    # saves MCAP bag to recordings/
scripts/flychams.py sim --record --record-name foo  # saves to recordings/foo/foo.mcap
```

### With duration limit

```bash
scripts/flychams.py sim --duration 180          # stop after 3 minutes
scripts/flychams.py sim --record --duration 60  # record for 1 minute
```

### Individual scripts

```bash
scripts/launch_px4.sh 0                  # PX4 SITL
scripts/launch_micro_xrce_dds.sh         # Micro-XRCE-DDS
scripts/launch_coordinator.sh            # Coordinator
scripts/launch_simulation.sh             # Simulation
scripts/launch_agent.sh AGENT00          # Agent
scripts/launch_operator.sh               # Operator
```

---

## 2. Operator Interface

```bash
scripts/launch_operator.sh
```

Connect Foxglove Studio → **Open connection** → **Foxglove WebSocket** → `ws://<host-ip>:8765`. Import the layout via **File → Import layout from file** → select `src/flychams_common/config/generated/flychams.json`. See [foxglove.md](foxglove.md) for details.

---

## 3. Stop

```bash
scripts/stop.sh
```

---

## 4. Logs

```bash
scripts/logs.sh                        # all containers
docker logs -f flychams-coordinator    # single container
```