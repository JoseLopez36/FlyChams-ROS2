#include "flychams_agent/state/agent_state.hpp"

using namespace flychams::common;

using namespace flychams::agent;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void DroneState::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);
    // Get flight parameters
    takeoff_altitude_ = node_->getParameterOr<float>("takeoff_altitude", 3.0f);
    landing_altitude_ = node_->getParameterOr<float>("landing_altitude", 0.5f);

    // Initialize data
    agent_ = Agent();

    // Create PX4 communication
    autopilot_comm_ = std::make_shared<AutopilotCommunication>(agent_id_, node_);

    // Subscribe to PX4 topics
    agent_.status_sub = autopilot_comm_->subscribeVehicleStatus(
        std::bind(&DroneState::vehicleStatusCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());
    agent_.local_odom_sub = autopilot_comm_->subscribeLocalOdometry(
        std::bind(&DroneState::localOdomCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Create publishers for agent status and position
    agent_.status_pub = node_->createAgentStatusPublisher(agent_id_);
    agent_.local_position_pub = node_->createAgentLocalPositionPublisher(agent_id_);
    agent_.global_position_pub = node_->createAgentGlobalPositionPublisher(agent_id_);

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&DroneState::update, this));
}

void DroneState::onModuleShutdown()
{
    // Destroy PX4 subscribers
    agent_.status_sub.reset();
    agent_.local_odom_sub.reset();
    // Destroy publishers
    agent_.status_pub.reset();
    agent_.local_position_pub.reset();
    agent_.global_position_pub.reset();
    // Destroy PX4 communication
    autopilot_comm_.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void DroneState::vehicleStatusCallback(const px4_msgs::msg::VehicleStatus::SharedPtr msg)
{
    agent_.vehicle_status = *msg;
    agent_.has_status = true;
}

void DroneState::localOdomCallback(const px4_msgs::msg::VehicleOdometry::SharedPtr msg)
{
    agent_.vehicle_odom = *msg;
    agent_.has_local_odom = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update state
// ════════════════════════════════════════════════════════════════════════════

void DroneState::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Drone state: Skipping update due to invalid status");
        return;
    }

    // Handle state based on current odometry and status
    // VehicleStatus: arming_state == 2 means ARMED
    bool connected = true;
    bool armed = (agent_.vehicle_status.arming_state == px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED);
    // VehicleOdometry position[2] is NED down — negate for altitude (up positive)
    float altitude = -agent_.vehicle_odom.position[2];

    // Map PX4 state to simplified 3-state AgentStatus
    // ACTIVE = armed AND flying (above takeoff altitude)
    AgentStatus status = AgentStatus::IDLE;
    bool is_flying = false;

    if (!connected)
    {
        status = AgentStatus::ERROR;
        RCLCPP_ERROR(node_->get_logger(), "Drone state: Agent %s is not connected", agent_id_.c_str());
    }
    else if (!armed)
    {
        // Disarmed = IDLE (safe state on ground)
        status = AgentStatus::IDLE;
        RCLCPP_INFO(node_->get_logger(), "Drone state: Agent %s is disarmed", agent_id_.c_str());
    }
    else
    {
        // Armed: check if flying above takeoff altitude
        is_flying = altitude >= takeoff_altitude_;
        if (is_flying)
        {
            status = AgentStatus::ACTIVE;
            RCLCPP_INFO(node_->get_logger(), "Drone state: Agent %s is flying", agent_id_.c_str());
        }
        else
        {
            // Armed but still on ground (taking off or just armed on ground)
            status = AgentStatus::IDLE;
            RCLCPP_INFO(node_->get_logger(), "Drone state: Agent %s is armed but not flying", agent_id_.c_str());
        }
    }

    // Publish agent status
    AgentStatusMsg status_msg;
    status_msg.header.stamp = node_->now();
    status_msg.status = static_cast<uint8_t>(status);
    status_msg.is_armed = armed;
    status_msg.is_flying = is_flying;
    agent_.status_pub->publish(status_msg);

    // Update local and global position
    updateLocalPosition(agent_.vehicle_odom);
    updateGlobalPosition(agent_.vehicle_odom);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool DroneState::checkStatus()
{
    // Check 1: Agent must have a valid PX4 vehicle status
    if (!agent_.has_status)
    {
        RCLCPP_WARN(node_->get_logger(), "Drone state: Agent %s has no vehicle status", agent_id_.c_str());
        return false;
    }

    // Check 2: Agent must have a valid PX4 local odometry
    if (!agent_.has_local_odom)
    {
        RCLCPP_WARN(node_->get_logger(), "Drone state: Agent %s has no local odometry", agent_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS UPDATE: Status update
// ════════════════════════════════════════════════════════════════════════════

void DroneState::updateLocalPosition(const px4_msgs::msg::VehicleOdometry& vehicle_odom)
{
    // Convert NED position to ENU
    const Vector3r ned_pos(vehicle_odom.position[0], vehicle_odom.position[1], vehicle_odom.position[2]);
    const Vector3r enu_pos = FrameUtils::pointFromNED(ned_pos);

    PointStampedMsg local_position_msg;
    local_position_msg.header.stamp = node_->now();
    local_position_msg.header.frame_id = node_->getAgentLocalFrame(agent_id_);
    local_position_msg.point.x = enu_pos.x();
    local_position_msg.point.y = enu_pos.y();
    local_position_msg.point.z = enu_pos.z();

    agent_.local_position_pub->publish(local_position_msg);
}

void DroneState::updateGlobalPosition(const px4_msgs::msg::VehicleOdometry& vehicle_odom)
{
    // Convert NED position to ENU
    const Vector3r ned_pos(vehicle_odom.position[0], vehicle_odom.position[1], vehicle_odom.position[2]);
    const Vector3r enu_pos = FrameUtils::pointFromNED(ned_pos);

    PointStampedMsg local_position_msg;
    local_position_msg.header.stamp = node_->now();
    local_position_msg.header.frame_id = node_->getAgentLocalFrame(agent_id_);
    local_position_msg.point.x = enu_pos.x();
    local_position_msg.point.y = enu_pos.y();
    local_position_msg.point.z = enu_pos.z();

    PointStampedMsg global_position_msg;
    global_position_msg.header.stamp = node_->now();
    global_position_msg = node_->transformPoint(local_position_msg, node_->getGlobalFrame());

    agent_.global_position_pub->publish(global_position_msg);
}