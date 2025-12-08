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

        // Get flight parameters
        takeoff_altitude_ = RosUtils::getParameterOr<float>(node_, "drone_state.takeoff_altitude", 1.5f);
        landing_altitude_ = RosUtils::getParameterOr<float>(node_, "drone_state.landing_altitude", 0.5f);

        // Initialize data
        agent_ = Agent();

        // Set initial status as IDLE
        agent_.status_out = AgentStatus::IDLE;

        // Initialize message data
        agent_.status_out_msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        agent_.local_position_out_msg.header = RosUtils::createHeader(node_, transform_tools_->getAgentLocalFrame(agent_id_));
        agent_.global_position_out_msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());

        // Create mavros communication
        mavros_comm_ = std::make_shared<MavrosCommunication>(agent_id_, node_, config_tools_, topic_tools_, transform_tools_, module_cb_group_);

        // Subscribe to mavros topics
        agent_.status_in_sub = mavros_comm_->subscribeStatus(
            std::bind(&DroneState::statusInCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.local_odom_in_sub = mavros_comm_->subscribeLocalOdometry(
            std::bind(&DroneState::localOdomCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.global_odom_in_sub = mavros_comm_->subscribeGlobalOdometry(
            std::bind(&DroneState::globalOdomCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Create publisher for agent status and position
        agent_.status_out_pub = topic_tools_->createAgentStatusPublisher(agent_id_);
        agent_.local_position_out_pub = topic_tools_->createAgentLocalPositionPublisher(agent_id_);
        agent_.global_position_out_pub = topic_tools_->createAgentGlobalPositionPublisher(agent_id_);

        // Set update timer
        last_update_time_ = RosUtils::now(node_);
        update_timer_ = RosUtils::createTimer(node_, update_rate_,
            std::bind(&DroneState::update, this), module_cb_group_);
    }

    void DroneState::onShutdown()
    {
        // Destroy subscriber
        agent_.status_in_sub.reset();
        agent_.local_odom_in_sub.reset();
        agent_.global_odom_in_sub.reset();
        // Destroy publisher
        agent_.status_out_pub.reset();
        agent_.local_position_out_pub.reset();
        agent_.global_position_out_pub.reset();
        // Destroy mavros communication
        mavros_comm_.reset();
        // Destroy update timer
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void DroneState::statusInCallback(const mavros_msgs::msg::State::SharedPtr msg)
    {
        // Update current status
        agent_.status_in_msg = *msg;
        agent_.has_status_in = true;
    }

    void DroneState::localOdomCallback(const core::OdometryMsg::SharedPtr msg)
    {
        // Update current odometry
        agent_.local_odom_in_msg = *msg;
        agent_.has_local_odom_in = true;

        // Publish agent position in local frame
        agent_.local_position_out_msg.header = msg->header;
        agent_.local_position_out_msg.point = msg->pose.pose.position;
        agent_.local_position_out_pub->publish(agent_.local_position_out_msg);
    }

    void DroneState::globalOdomCallback(const core::OdometryMsg::SharedPtr msg)
    {
        // Update current odometry
        agent_.global_odom_in_msg = *msg;
        agent_.has_global_odom_in = true;

        // Publish agent position in global frame
        agent_.global_position_out_msg.header = msg->header;
        agent_.global_position_out_msg.point = msg->pose.pose.position;
        agent_.global_position_out_pub->publish(agent_.global_position_out_msg);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATE MANAGEMENT: State transition and validation methods
    // ════════════════════════════════════════════════════════════════════════════

    void DroneState::update()
    {
        // Check if we have a valid odometry and status
        if (!agent_.has_local_odom_in || !agent_.has_global_odom_in || !agent_.has_status_in)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone state: No odometry or status data received for agent %s", agent_id_.c_str());
            return;
        }

        // Compute time step
        auto current_time = RosUtils::now(node_);
        float dt = (current_time - last_update_time_).seconds();
        last_update_time_ = current_time;

        // Handle state based on current odometry and status
        bool connected = agent_.status_in_msg.connected;
        bool armed = agent_.status_in_msg.armed;
        std::string mode = agent_.status_in_msg.mode;
        float altitude = agent_.local_odom_in_msg.pose.pose.position.z;

        if (!connected)
        {
            agent_.status_out = AgentStatus::ERROR;
            RCLCPP_ERROR(node_->get_logger(), "Drone state: Agent %s is not connected", agent_id_.c_str());
        }
        else if (!armed)
        {
            agent_.status_out = AgentStatus::IDLE;
            RCLCPP_WARN(node_->get_logger(), "Drone state: Agent %s is not armed", agent_id_.c_str());
        }
        else
        {
            if (mode == "AUTO.TAKEOFF")
            {
                agent_.status_out = AgentStatus::TAKEOFF;
            }
            else if (mode == "AUTO.LAND" || mode == "AUTO.RTL")
            {
                agent_.status_out = AgentStatus::LAND;
            }
            else
            {
                // OFFBOARD, AUTO.MISSION, AUTO.LOITER, POSCTL, etc.
                if (altitude >= takeoff_altitude_ - 0.5f)
                {
                    agent_.status_out = AgentStatus::MISSION;
                }
                else
                {
                    agent_.status_out = AgentStatus::TAKEOFF;
                }
            }
        }

        // Publish agent status
        agent_.status_out_msg.header.stamp = RosUtils::now(node_);
        agent_.status_out_msg.status = static_cast<uint8_t>(agent_.status_out);
        agent_.status_out_pub->publish(agent_.status_out_msg);
    }

} // namespace flychams::control
