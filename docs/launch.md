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

## 4. Rosbag Replay (Benchmarking)

Use this procedure to replay a previously recorded bag to benchmark a subsystem. For example, to benchmark the assignment solver (e.g. compare `EXHAUSTIVE_SEARCH` vs `BRANCH_AND_BOUND` node counts) without running PX4 or AirSim.

**1. Start the operator container and replay the bag inside it:**

```bash
CMD="source install/setup.bash && ros2 bag play recordings/<bag-name>/<bag-name>.mcap --clock --topics \
        /tf \
        /tf_static \
        /flychams/coordinator/registration \
        /flychams/coordinator/mission_status \
        /flychams/coordinator/fleet_status \
        /flychams/coordinator/CLUSTER00/geometry \
        /flychams/coordinator/CLUSTER01/geometry \
        /flychams/coordinator/CLUSTER02/geometry \
        /flychams/agent/AGENT00/global_position \
        /flychams/agent/AGENT01/global_position \
        /flychams/agent/AGENT02/global_position" \
scripts/docker/run_operator.sh
```

> Adjust the `CLUSTER*` and `AGENT*` entries to match the IDs in your recording. `--loop` keeps replaying so the coordinator can run continuously.

**2. In a second terminal, start only the coordinator container:**

```bash
scripts/launch_coordinator.sh
```

**3. Monitor the solver output topics:**

```bash
# Solve duration (ms)
ros2 topic echo /flychams/coordinator/assignment_solve_duration

# Evaluated node count
ros2 topic echo /flychams/coordinator/assignment_node_count
```

**4. Optionally record the benchmark results into a new bag:**

```bash
CMD="source install/setup.bash && ros2 bag record --storage mcap \
    --output recordings/benchmark_\$(date +%Y%m%d_%H%M%S) \
    /flychams/coordinator/assignment_solve_duration \
    /flychams/coordinator/assignment_node_count" \
scripts/docker/exec_operator.sh
```

---

## 5. Logs

```bash
scripts/logs.sh                        # all containers
docker logs -f flychams-coordinator    # single container
```