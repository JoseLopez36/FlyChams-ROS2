#include "flychams_agent/autopilot/autopilot_communication.hpp"

#include <limits>

using namespace flychams::common;
using namespace flychams::agent;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void AutopilotCommunication::onModuleInit()
{
    rclcpp::QoS qos_pub = rclcpp::QoS(rclcpp::KeepLast(1))
        .reliability(rclcpp::ReliabilityPolicy::BestEffort)
        .durability(rclcpp::DurabilityPolicy::TransientLocal);

    vehicle_command_pub_ = node_->create_publisher<px4_msgs::msg::VehicleCommand>(
        "/" + agent_id_ + "/fmu/in/vehicle_command", qos_pub);
    offboard_control_mode_pub_ = node_->create_publisher<px4_msgs::msg::OffboardControlMode>(
        "/" + agent_id_ + "/fmu/in/offboard_control_mode", qos_pub);
    trajectory_setpoint_pub_ = node_->create_publisher<px4_msgs::msg::TrajectorySetpoint>(
        "/" + agent_id_ + "/fmu/in/trajectory_setpoint", qos_pub);
}

void AutopilotCommunication::onModuleShutdown()
{
    vehicle_command_pub_.reset();
    offboard_control_mode_pub_.reset();
    trajectory_setpoint_pub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// VEHICLE STATE
// ════════════════════════════════════════════════════════════════════════════

SubscriberPtr<px4_msgs::msg::HomePosition> AutopilotCommunication::subscribeHomePosition(
    const std::function<void(const px4_msgs::msg::HomePosition::SharedPtr)>& callback,
    const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1))
        .reliability(rclcpp::ReliabilityPolicy::BestEffort)
        .durability(rclcpp::DurabilityPolicy::TransientLocal);
    return node_->create_subscription<px4_msgs::msg::HomePosition>(
        "/" + agent_id_ + "/fmu/out/home_position", qos, callback, options);
}

SubscriberPtr<px4_msgs::msg::VehicleStatus> AutopilotCommunication::subscribeVehicleStatus(
    const std::function<void(const px4_msgs::msg::VehicleStatus::SharedPtr)>& callback,
    const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1))
        .reliability(rclcpp::ReliabilityPolicy::BestEffort)
        .durability(rclcpp::DurabilityPolicy::Volatile);
    return node_->create_subscription<px4_msgs::msg::VehicleStatus>(
        "/" + agent_id_ + "/fmu/out/vehicle_status", qos, callback, options);
}

SubscriberPtr<px4_msgs::msg::VehicleOdometry> AutopilotCommunication::subscribeLocalOdometry(
    const std::function<void(const px4_msgs::msg::VehicleOdometry::SharedPtr)>& callback,
    const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1))
        .reliability(rclcpp::ReliabilityPolicy::BestEffort)
        .durability(rclcpp::DurabilityPolicy::Volatile);
    return node_->create_subscription<px4_msgs::msg::VehicleOdometry>(
        "/" + agent_id_ + "/fmu/out/vehicle_odometry", qos, callback, options);
}

// ════════════════════════════════════════════════════════════════════════════
// VEHICLE CONTROL
// ════════════════════════════════════════════════════════════════════════════

bool AutopilotCommunication::armDisarm(const bool& arm)
{
    publishVehicleCommand(
        px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM,
        arm ? 1.0f : 0.0f);
    return true;
}

bool AutopilotCommunication::takeoff(const float& z)
{
    publishVehicleCommand(
        px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_TAKEOFF,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, z);
    return true;
}

bool AutopilotCommunication::land()
{
    publishVehicleCommand(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);
    return true;
}

bool AutopilotCommunication::setMode(const std::string& mode)
{
    if (mode == "AUTO.RTL")
    {
        publishVehicleCommand(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE, 1.0f, 5.0f);
    }
    else if (mode == "AUTO.LOITER")
    {
        publishVehicleCommand(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE, 1.0f, 4.0f);
    }
    return true;
}

bool AutopilotCommunication::enableOffboard(const bool& enable)
{
    if (enable)
    {
        px4_msgs::msg::OffboardControlMode offboard_msg{};
        offboard_msg.timestamp = node_->now().nanoseconds() / 1000;
        offboard_msg.position = true;
        offboard_msg.velocity = false;
        offboard_msg.acceleration = false;
        offboard_control_mode_pub_->publish(offboard_msg);

        publishVehicleCommand(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE, 1.0f, 6.0f);
    }
    else
    {
        setMode("AUTO.LOITER");
    }
    return true;
}

void AutopilotCommunication::setLocalPosition(const float& x, const float& y, const float& z)
{
    px4_msgs::msg::OffboardControlMode offboard_msg{};
    offboard_msg.timestamp = node_->now().nanoseconds() / 1000;
    offboard_msg.position = true;
    offboard_msg.velocity = false;
    offboard_msg.acceleration = false;
    offboard_control_mode_pub_->publish(offboard_msg);

    // Input is ENU (local frame) — convert to NED for PX4 TrajectorySetpoint
    const Vector3r ned = FrameUtils::pointToNED(Vector3r(x, y, z));

    px4_msgs::msg::TrajectorySetpoint setpoint_msg{};
    setpoint_msg.timestamp = node_->now().nanoseconds() / 1000;
    setpoint_msg.position[0] = static_cast<float>(ned.x());
    setpoint_msg.position[1] = static_cast<float>(ned.y());
    setpoint_msg.position[2] = static_cast<float>(ned.z());
    setpoint_msg.yaw = std::numeric_limits<float>::quiet_NaN();
    trajectory_setpoint_pub_->publish(setpoint_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// HELPERS
// ════════════════════════════════════════════════════════════════════════════

void AutopilotCommunication::publishVehicleCommand(uint32_t command, float param1, float param2,
                                             float param3, float param4,
                                             float param5, float param6, float param7)
{
    px4_msgs::msg::VehicleCommand msg{};
    msg.timestamp = node_->now().nanoseconds() / 1000;
    msg.command = command;
    msg.param1 = param1;
    msg.param2 = param2;
    msg.param3 = param3;
    msg.param4 = param4;
    msg.param5 = param5;
    msg.param6 = param6;
    msg.param7 = param7;
    msg.target_system = 1;
    msg.target_component = 1;
    msg.source_system = 1;
    msg.source_component = 1;
    msg.from_external = true;
    vehicle_command_pub_->publish(msg);
}