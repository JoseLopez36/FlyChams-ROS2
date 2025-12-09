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
        // Get flight parameters
        takeoff_altitude_ = RosUtils::getParameterOr<float>(node_, "takeoff_altitude", 1.5f);
        landing_altitude_ = RosUtils::getParameterOr<float>(node_, "landing_altitude", 0.5f);

        // Initialize data
        agent_ = Agent();

        // Create mavros communication
        mavros_comm_ = std::make_shared<MavrosCommunication>(agent_id_, node_, config_tools_, topic_tools_, transform_tools_, module_cb_group_);

        // Subscribe to mavros topics
        agent_.state_sub = mavros_comm_->subscribeState(
            std::bind(&DroneState::stateCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.local_odom_sub = mavros_comm_->subscribeLocalOdometry(
            std::bind(&DroneState::localOdomCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Create publishers for agent status and position
        agent_.status_pub = topic_tools_->createAgentStatusPublisher(agent_id_);
        agent_.local_position_pub = topic_tools_->createAgentLocalPositionPublisher(agent_id_);
        agent_.global_position_pub = topic_tools_->createAgentGlobalPositionPublisher(agent_id_);
    }

    void DroneState::onShutdown()
    {
        // Destroy subscribers
        agent_.state_sub.reset();
        agent_.local_odom_sub.reset();
        // Destroy publishers
        agent_.status_pub.reset();
        agent_.local_position_pub.reset();
        agent_.global_position_pub.reset();
        // Destroy mavros communication
        mavros_comm_.reset();
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

    void DroneState::localOdomCallback(const core::OdometryMsg::SharedPtr msg)
    {
        // Check if we have a valid state
        if (!agent_.has_state)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: No state data received for agent %s", agent_id_.c_str());
            return;
        }

        // Get odometry
        const OdometryMsg& local_odom = *msg;

        // Update status
        updateStatus(agent_.state, local_odom);

        // Update local and global position
        updateLocalPosition(local_odom);
        updateGlobalPosition(local_odom);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATE MANAGEMENT: State transition and validation methods
    // ════════════════════════════════════════════════════════════════════════════

    void DroneState::updateStatus(const mavros_msgs::msg::State& state, const OdometryMsg& local_odom)
    {
        // Handle state based on current odometry and status
        bool connected = state.connected;
        bool armed = state.armed;
        std::string mode = state.mode;
        float altitude = local_odom.pose.pose.position.z;

        // Initialize status
        AgentStatus status = AgentStatus::IDLE;

        if (!connected)
        {
            status = AgentStatus::ERROR;
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Agent %s is not connected", agent_id_.c_str());
        }
        else if (!armed)
        {
            status = AgentStatus::IDLE;
            RCLCPP_WARN(node_->get_logger(), "Drone state: Agent %s is not armed", agent_id_.c_str());
        }
        else
        {
            if (mode == "AUTO.TAKEOFF")
            {
                status = AgentStatus::TAKEOFF;
            }
            else if (mode == "AUTO.LAND" || mode == "AUTO.RTL")
            {
                status = AgentStatus::LAND;
            }
            else
            {
                // OFFBOARD, AUTO.MISSION, AUTO.LOITER, POSCTL, etc.
                if (altitude >= takeoff_altitude_ - 0.5f)
                {
                    status = AgentStatus::MISSION;
                }
                else
                {
                    status = AgentStatus::TAKEOFF;
                }
            }
        }

        // Publish agent status
        AgentStatusMsg status_msg;
        status_msg.header.stamp = RosUtils::now(node_);
        status_msg.status = static_cast<uint8_t>(status);
        agent_.status_pub->publish(status_msg);
    }

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
        global_position_msg.header.frame_id = transform_tools_->getGlobalFrame();
        global_position_msg = transform_tools_->transformPoint(local_position_msg, transform_tools_->getGlobalFrame());

        // Publish agent global position
        agent_.global_position_pub->publish(global_position_msg);
    }

} // namespace flychams::control
