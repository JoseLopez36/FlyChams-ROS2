# Autopilot

FlyChams uses **PX4 v1.16** for SITL flight control and **Micro-XRCE-DDS Agent v2.4.3** to bridge PX4 uORB topics to ROS2.

> References: [PX4 v1.16 docs](https://docs.px4.io/v1.16/en/) · [Micro-XRCE-DDS](https://micro-xrce-dds.docs.eprosima.com/)

---

## Environment Variables

Add to your shell profile:

```bash
export PX4_AUTOPILOT_PATH=/path/to/PX4-Autopilot
export Micro_XRCE_DDS_AGENT_PATH=/path/to/Micro-XRCE-DDS-Agent
```

---

## 1. PX4

```bash
# Clone
git clone --branch v1.16.0 --recursive https://github.com/PX4/PX4-Autopilot.git

# Pull the build toolchain image
docker pull px4io/px4-dev-nuttx-focal:2022-08-12

# Copy the FlyChams DDS topic config (minimal topic set)
cp src/flychams_agent/config/dds_topics.yaml \
   $PX4_AUTOPILOT_PATH/src/modules/uxrce_dds_client/dds_topics.yaml

# Build SITL firmware
cd $PX4_AUTOPILOT_PATH
./Tools/docker_run.sh "make px4_sitl_default none_iris"
```

---

## 2. Micro-XRCE-DDS Agent

```bash
# Clone
git clone --branch v2.4.3 --recursive https://github.com/eProsima/Micro-XRCE-DDS-Agent.git

# Build Docker image
cd $Micro_XRCE_DDS_AGENT_PATH
docker build -t micro-xrce-dds-agent .
```

---

## Topic Namespacing

Each PX4 instance runs with `PX4_UXRCE_DDS_NS=<AGENT_ID>`, producing namespaced topics:

```
/AGENT00/fmu/out/home_position
/AGENT00/fmu/out/vehicle_odometry
/AGENT00/fmu/out/vehicle_status
/AGENT00/fmu/in/offboard_control_mode
/AGENT00/fmu/in/trajectory_setpoint
/AGENT00/fmu/in/vehicle_command
```

Both PX4 and the DDS agent are launched automatically by `scripts/flychams.py sim`. See [launch.md](launch.md).