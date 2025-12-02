#include "flychams_control/drone/drone_state.hpp"

using namespace flychams::core;

namespace flychams::control
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void DroneState::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "drone_state.update_rate", 5.0f);
        // Get takeoff parameters
        takeoff_altitude_ = RosUtils::getParameterOr<float>(node_, "drone_state.takeoff_altitude", 1.5f);
        takeoff_timeout_ = RosUtils::getParameterOr<float>(node_, "drone_state.takeoff_timeout", 60.0f);
        // Get landing parameters
        landing_altitude_ = RosUtils::getParameterOr<float>(node_, "drone_state.landing_altitude", 0.5f);
        landing_timeout_ = RosUtils::getParameterOr<float>(node_, "drone_state.landing_timeout", 10.0f);
        // Get hover parameters
        hover_altitude_ = RosUtils::getParameterOr<float>(node_, "drone_state.hover_altitude", 1.0f);
        hover_timeout_ = RosUtils::getParameterOr<float>(node_, "drone_state.hover_timeout", 5.0f);

        // Compute command timeout
        cmd_timeout_ = (1.0f / update_rate_) * 1.25f;

        // Initialize data
        agent_ = Agent();

        // Set initial status as IDLE
        agent_.status = AgentStatus::IDLE;

        // Initialize message data
        agent_.status_msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        agent_.position_msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());

        // Create mavros communication
        mavros_comm_ = std::make_shared<MavrosCommunication>(agent_id_, node_, config_tools_, transform_tools_);

        // Subscribe to agent odom topic from framework tools
        agent_.odom_sub = framework_tools_->createOdometrySubscriber(agent_id_,
            std::bind(&DroneState::odomCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Create publisher for agent status and position
        agent_.status_pub = topic_tools_->createAgentStatusPublisher(agent_id_);
        agent_.position_pub = topic_tools_->createAgentPositionPublisher(agent_id_);

        // Set update timer
        status_duration_ = 0.0f;
        last_update_time_ = RosUtils::now(node_);
        update_timer_ = RosUtils::createTimer(node_, update_rate_,
            std::bind(&DroneState::update, this), module_cb_group_);
    }

    void DroneState::onShutdown()
    {
        // Destroy subscriber
        agent_.odom_sub.reset();
        // Destroy publisher
        agent_.status_pub.reset();
        agent_.position_pub.reset();
        // Destroy update timer
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // PUBLIC METHODS: Methods for initializing and handling the state manager
    // ════════════════════════════════════════════════════════════════════════════

    bool DroneState::requestDisarm()
    {
        // Check if we're in ARMED, LANDED or ERROR state
        if (agent_.status != AgentStatus::ARMED && agent_.status != AgentStatus::LANDED && agent_.status != AgentStatus::ERROR)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Cannot disarm agent %s from state %d", agent_id_.c_str(), static_cast<int>(agent_.status));
            return false;
        }

        // Enable OFFBOARD mode
        bool enable_result = mavros_comm_->enableOffboard(true);
        if (!enable_result)
        {
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Failed to enable control for agent %s", agent_id_.c_str());
            return false;
        }

        // Request disarming
        RCLCPP_INFO(node_->get_logger(), "Drone state: Disarming agent %s...", agent_id_.c_str());
        bool disarm_result = mavros_comm_->armDisarm(false);
        if (!disarm_result)
        {
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Failed to disarm agent %s", agent_id_.c_str());
            return false;
        }

        // Transition to IDLE state
        setStatus(AgentStatus::IDLE);
        return true;
    }

    bool DroneState::requestArm()
    {
        // Check if we're in IDLE state
        if (agent_.status != AgentStatus::IDLE)
        {
            // Only warn if not already ARMED (to avoid spamming if called redundantly)
            if (agent_.status != AgentStatus::ARMED)
                RCLCPP_WARN(node_->get_logger(), "Drone state: Cannot arm agent %s from state %d", agent_id_.c_str(), static_cast<int>(agent_.status));
            return false;
        }

        // Request arming
        RCLCPP_INFO(node_->get_logger(), "Drone state: Arming agent %s...", agent_id_.c_str());
        bool arm_result = mavros_comm_->armDisarm(true);
        if (!arm_result)
        {
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Failed to arm agent %s", agent_id_.c_str());
            return false;
        }

        // Transition to ARMED state
        setStatus(AgentStatus::ARMED);
        return true;
    }

    bool DroneState::requestTakeoff()
    {
        // Check if we're in ARMED state
        if (agent_.status != AgentStatus::ARMED && agent_.status != AgentStatus::TAKING_OFF)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Cannot takeoff agent %s from state %d", agent_id_.c_str(), static_cast<int>(agent_.status));
            return false;
        }

        // Request takeoff
        RCLCPP_INFO(node_->get_logger(), "Drone state: Taking off agent %s...", agent_id_.c_str());
        bool takeoff_result = mavros_comm_->takeoff(takeoff_altitude_);
        if (!takeoff_result)
        {
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Failed to takeoff agent %s", agent_id_.c_str());
            return false;
        }

        // Transition to TAKING_OFF state
        setStatus(AgentStatus::TAKING_OFF);
        return true;
    }

    bool DroneState::requestHover()
    {
        // Allowed from TAKING_OFF (completion), TRACKING (return), or HOVERING (retry)
        if (agent_.status != AgentStatus::TAKING_OFF && agent_.status != AgentStatus::TRACKING && agent_.status != AgentStatus::HOVERING)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Cannot hover agent %s from state %d", agent_id_.c_str(), static_cast<int>(agent_.status));
            return false;
        }

        // Enable OFFBOARD mode
        bool enable_result = mavros_comm_->enableOffboard(true);
        if (!enable_result)
        {
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Failed to enable control for agent %s", agent_id_.c_str());
            return false;
        }

        // Request hover
        RCLCPP_INFO(node_->get_logger(), "Drone state: Hovering agent %s...", agent_id_.c_str());
        mavros_comm_->setPosition(agent_.odom.pose.pose.position.x, agent_.odom.pose.pose.position.y, hover_altitude_);

        // Transition to HOVERING state
        setStatus(AgentStatus::HOVERING);
        return true;
    }

    bool DroneState::requestTracking()
    {
        // Check if we're in HOVERING state or already TRACKING
        if (agent_.status != AgentStatus::HOVERING && agent_.status != AgentStatus::TRACKING)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Cannot move agent %s from state %d", agent_id_.c_str(), static_cast<int>(agent_.status));
            return false;
        }

        // Enable OFFBOARD mode
        bool enable_result = mavros_comm_->enableOffboard(true);
        if (!enable_result)
        {
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Failed to enable control for agent %s", agent_id_.c_str());
            return false;
        }

        // Request move
        RCLCPP_INFO(node_->get_logger(), "Drone state: Agent %s moving to goal...", agent_id_.c_str());

        // Transition to TRACKING state
        setStatus(AgentStatus::TRACKING);
        return true;
    }

    bool DroneState::requestLand()
    {
        // Check if we're in HOVERING, TRACKING, TAKING_OFF, or LANDING
        if (agent_.status != AgentStatus::HOVERING && agent_.status != AgentStatus::TRACKING && agent_.status != AgentStatus::TAKING_OFF && agent_.status != AgentStatus::LANDING && agent_.status != AgentStatus::ERROR)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Cannot land agent %s from state %d", agent_id_.c_str(), static_cast<int>(agent_.status));
            return false;
        }

        // Request landing
        RCLCPP_INFO(node_->get_logger(), "Drone state: Landing agent %s...", agent_id_.c_str());
        bool land_result = mavros_comm_->land();
        if (!land_result)
        {
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Failed to land agent %s", agent_id_.c_str());
            return false;
        }

        // Transition to LANDING state
        setStatus(AgentStatus::LANDING);
        return true;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void DroneState::odomCallback(const core::OdometryMsg::SharedPtr msg)
    {
        // Update current odometry
        agent_.odom = *msg;
        agent_.has_odom = true;

        // Publish agent position
        agent_.position_msg.header.stamp = RosUtils::now(node_);
        agent_.position_msg.point = agent_.odom.pose.pose.position;
        agent_.position_pub->publish(agent_.position_msg);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATE MANAGEMENT: State transition and validation methods
    // ════════════════════════════════════════════════════════════════════════════

    void DroneState::update()
    {
        // Check if we have a valid position. If not, we can't update the state (except IDLE)
        if (!agent_.has_odom && agent_.status != AgentStatus::IDLE)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: No odometry data received for agent %s", agent_id_.c_str());
            return;
        }

        // Compute time step
        auto current_time = RosUtils::now(node_);
        float dt = (current_time - last_update_time_).seconds();
        last_update_time_ = current_time;

        // Limit dt to prevent extreme values after pauses
        dt = std::min(dt, cmd_timeout_);

        // Update state duration
        status_duration_ += dt;

        // Handle state based on current state
        switch (agent_.status)
        {
        case AgentStatus::IDLE:
            handleIdle();
            break;

        case AgentStatus::ARMED:
            handleArmed();
            break;

        case AgentStatus::TAKING_OFF:
            handleTakingOff();
            break;

        case AgentStatus::HOVERING:
            handleHovering();
            break;

        case AgentStatus::TRACKING:
            handleTracking();
            break;

        case AgentStatus::LANDING:
            handleLanding();
            break;

        case AgentStatus::LANDED:
            handleLanded();
            break;

        case AgentStatus::ERROR:
            handleError();
            break;

        default:
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Unknown state for agent %s: %d", agent_id_.c_str(), static_cast<int>(agent_.status));
            break;
        }

        // Publish agent status
        agent_.status_msg.header.stamp = RosUtils::now(node_);
        agent_.status_msg.status = static_cast<uint8_t>(agent_.status);
        agent_.status_pub->publish(agent_.status_msg);
    }

    void DroneState::setStatus(const AgentStatus& new_status)
    {
        // Check if this is a valid transition
        if (!isValid(agent_.status, new_status))
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Invalid state transition from %d to %d for agent %s",
                static_cast<int>(agent_.status), static_cast<int>(new_status), agent_id_.c_str());
            return;
        }

        // Set new state
        AgentStatus old_status = agent_.status;
        agent_.status = new_status;

        // Update state duration
        status_duration_ = 0.0f;

        // Log state transition
        RCLCPP_INFO(node_->get_logger(), "Drone state: Agent status transition from %d to %d for agent %s",
            static_cast<int>(old_status), static_cast<int>(new_status), agent_id_.c_str());
    }

    bool DroneState::isValid(const AgentStatus& from, const AgentStatus& to) const
    {
        // Simple linear flow + error handling
        switch (from)
        {
        case AgentStatus::IDLE:
            return to == AgentStatus::ARMED || to == AgentStatus::ERROR;

        case AgentStatus::ARMED:
            return to == AgentStatus::TAKING_OFF || to == AgentStatus::IDLE || to == AgentStatus::ERROR;

        case AgentStatus::TAKING_OFF:
            return to == AgentStatus::HOVERING || to == AgentStatus::LANDING || to == AgentStatus::ERROR;

        case AgentStatus::HOVERING:
            return to == AgentStatus::TRACKING || to == AgentStatus::LANDING || to == AgentStatus::ERROR;

        case AgentStatus::TRACKING:
            return to == AgentStatus::HOVERING || to == AgentStatus::LANDING || to == AgentStatus::ERROR;

        case AgentStatus::LANDING:
            return to == AgentStatus::LANDED || to == AgentStatus::ERROR;

        case AgentStatus::LANDED:
            return to == AgentStatus::IDLE || to == AgentStatus::ERROR;

        case AgentStatus::ERROR:
            return to == AgentStatus::LANDING || to == AgentStatus::LANDED; // Try to land if error

        default:
            return false;
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATE HANDLERS: Methods for handling different states
    // ════════════════════════════════════════════════════════════════════════════

    void DroneState::handleIdle()
    {
        // Auto-start: try to arm
        requestArm();
    }

    void DroneState::handleArmed()
    {
        // Auto-continue: try to takeoff
        requestTakeoff();
    }

    void DroneState::handleTakingOff()
    {
        // Check timeout
        if (status_duration_ > takeoff_timeout_)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Takeoff timeout for agent %s, retrying...", agent_id_.c_str());
            requestTakeoff();
            return;
        }

        // Check altitude
        if (agent_.odom.pose.pose.position.z >= takeoff_altitude_)
        {
            RCLCPP_INFO(node_->get_logger(), "Drone state: Takeoff altitude reached for agent %s", agent_id_.c_str());
            // Transition to hovering
            requestHover();
        }
    }

    void DroneState::handleHovering()
    {
        // Check timeout
        if (status_duration_ > hover_timeout_)
        {
             // Transition to tracking after hover duration
            requestTracking();
            return;
        }

        // Could re-send hover command if needed, but we assume it's stable.
    }

    void DroneState::handleTracking()
    {
        // Tracking is handled by other nodes, we just stay here unless error or external command
    }

    void DroneState::handleLanding()
    {
        // Check timeout
        if (status_duration_ > landing_timeout_)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: Landing timeout for agent %s, retrying...", agent_id_.c_str());
            requestLand();
            return;
        }

        // Check altitude
        if (agent_.odom.pose.pose.position.z <= landing_altitude_)
        {
            RCLCPP_INFO(node_->get_logger(), "Drone state: Landing complete for agent %s", agent_id_.c_str());
            setStatus(AgentStatus::LANDED);
        }
    }

    void DroneState::handleLanded()
    {
        // After landing, we can disarm and go to IDLE
        requestDisarm();
    }

    void DroneState::handleError()
    {
        // In ERROR state, attempt landing
        if (agent_.odom.pose.pose.position.z > landing_altitude_)
        {
            requestLand();
        }
        else
        {
            setStatus(AgentStatus::LANDED);
        }
    }

} // namespace flychams::control
