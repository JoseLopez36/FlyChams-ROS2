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

    // Create mavros communication
    mavros_comm_ = std::make_shared<MavrosCommunication>(agent_id_, node_);

    // Subscribe to mavros topics
    agent_.state_sub = mavros_comm_->subscribeState(
        std::bind(&DroneState::stateCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());
    agent_.local_odom_sub = mavros_comm_->subscribeLocalOdometry(
        std::bind(&DroneState::localOdomCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Create publishers for agent status and position
    agent_.status_pub = node_->createAgentStatusPublisher(agent_id_);
    agent_.local_position_pub = node_->createAgentLocalPositionPublisher(agent_id_);
    agent_.global_position_pub = node_->createAgentGlobalPositionPublisher(agent_id_);

    // Subscribe to coordinator command topics
    arm_all_sub_ = node_->create_subscription<BoolMsg>(
        "/flychams/coordinator/arm_all", 10,
        std::bind(&DroneState::armAllCallback, this, std::placeholders::_1));
    return_home_sub_ = node_->create_subscription<BoolMsg>(
        "/flychams/coordinator/return_home", 10,
        std::bind(&DroneState::returnHomeCallback, this, std::placeholders::_1));

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&DroneState::update, this));
}

void DroneState::onModuleShutdown()
{
    // Destroy command subscribers
    arm_all_sub_.reset();
    return_home_sub_.reset();
    // Destroy mavros subscribers
    agent_.state_sub.reset();
    agent_.local_odom_sub.reset();
    // Destroy publishers
    agent_.status_pub.reset();
    agent_.local_position_pub.reset();
    agent_.global_position_pub.reset();
    // Destroy mavros communication
    mavros_comm_.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void DroneState::stateCallback(const mavros_msgs::msg::State::SharedPtr msg)
{
    // Update current status
    agent_.state = *msg;
    agent_.has_state = true;
}

void DroneState::localOdomCallback(const OdometryMsg::SharedPtr msg)
{
    // Store odometry data
    agent_.local_odom = *msg;
    agent_.has_local_odom = true;
}

void DroneState::armAllCallback(const BoolMsg::SharedPtr msg)
{
    bool requested_arm = msg->data;
    bool currently_armed = agent_.state.armed;

    if (requested_arm && !currently_armed)
    {
        RCLCPP_INFO(node_->get_logger(), "Arm command received for %s", agent_id_.c_str());
        armAgent(true);
    }
    else if (!requested_arm && currently_armed)
    {
        RCLCPP_INFO(node_->get_logger(), "Disarm command received for %s", agent_id_.c_str());
        armAgent(false);
    }
}

void DroneState::returnHomeCallback(const BoolMsg::SharedPtr msg)
{
    RCLCPP_INFO(node_->get_logger(), "Return home command received for %s", agent_id_.c_str());
    returnHome();
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update state
// ════════════════════════════════════════════════════════════════════════════

void DroneState::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        return;
    }

    // Handle state based on current odometry and status
    bool connected = agent_.state.connected;
    bool armed = agent_.state.armed;
    float altitude = agent_.local_odom.pose.pose.position.z;

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
    }
    else
    {
        // Armed: check if flying above takeoff altitude
        is_flying = altitude >= takeoff_altitude_;
        if (is_flying)
        {
            status = AgentStatus::ACTIVE;
        }
        else
        {
            // Armed but still on ground (taking off or just armed on ground)
            status = AgentStatus::IDLE;
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
    updateLocalPosition(agent_.local_odom);
    updateGlobalPosition(agent_.local_odom);
}

// ════════════════════════════════════════════════════════════════════════════
// COMMAND HANDLERS
// ════════════════════════════════════════════════════════════════════════════

void DroneState::armAgent(const bool arm)
{
    if (!mavros_comm_->armDisarm(arm))
    {
        RCLCPP_ERROR(node_->get_logger(), "Failed to %s %s", arm ? "arm" : "disarm", agent_id_.c_str());
    }
    else
    {
        RCLCPP_INFO(node_->get_logger(), "%s %s successfully", arm ? "Armed" : "Disarmed", agent_id_.c_str());
    }
}

void DroneState::returnHome()
{
    // Set mode to AUTO.RTL (Return to Launch)
    if (!mavros_comm_->setMode("AUTO.RTL"))
    {
        RCLCPP_ERROR(node_->get_logger(), "Failed to set RTL mode for %s", agent_id_.c_str());
    }
    else
    {
        RCLCPP_INFO(node_->get_logger(), "RTL mode set for %s", agent_id_.c_str());
    }
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool DroneState::checkStatus()
{
    // Check 1: Agent must have a valid mavros state
    if (!agent_.has_state)
    {
        return false;
    }

    // Check 2: Agent must have a valid mavros local odometry
    if (!agent_.has_local_odom)
    {
        return false;
    }

    // All checks passed
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS UPDATE: Status update
// ════════════════════════════════════════════════════════════════════════════

void DroneState::updateLocalPosition(const OdometryMsg& local_odom)
{
    // Create local position message
    PointStampedMsg local_position_msg;
    local_position_msg.header = local_odom.header;
    local_position_msg.point = local_odom.pose.pose.position;

    // Publish agent local position
    agent_.local_position_pub->publish(local_position_msg);
}

void DroneState::updateGlobalPosition(const OdometryMsg& local_odom)
{
    // Create local position message
    PointStampedMsg local_position_msg;
    local_position_msg.header = local_odom.header;
    local_position_msg.point = local_odom.pose.pose.position;

    // Create global position message
    PointStampedMsg global_position_msg;
    global_position_msg.header.stamp = local_odom.header.stamp;
    global_position_msg = node_->transformPoint(local_position_msg, node_->getGlobalFrame());

    // Publish agent global position
    agent_.global_position_pub->publish(global_position_msg);
}