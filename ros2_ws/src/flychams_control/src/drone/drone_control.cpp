#include "flychams_control/drone/drone_control.hpp"

using namespace flychams::core;

namespace flychams::control
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void DroneControl::onInit()
	{
		// Get parameters from parameter server
		// Get update rate
		update_rate_ = RosUtils::getParameterOr<float>(node_, "drone_control.update_rate", 10.0f);
		// Get control mode
		control_mode_ = static_cast<ControlMode>(RosUtils::getParameterOr<uint8_t>(node_, "drone_control.control_mode", 0));
		// Get speed planner parameters
		float min_speed = RosUtils::getParameterOr<float>(node_, "drone_control.min_speed", 0.5f);
		float max_speed = RosUtils::getParameterOr<float>(node_, "drone_control.max_speed", 12.0f);
		float min_distance = RosUtils::getParameterOr<float>(node_, "drone_control.min_distance", 0.20f);
		float max_distance = RosUtils::getParameterOr<float>(node_, "drone_control.max_distance", 50.0f);
		float max_acceleration = RosUtils::getParameterOr<float>(node_, "drone_control.max_acceleration", 2.0f);

		// Compute command timeout
		cmd_timeout_ = (1.0f / update_rate_) * 1.25f;

		// Initialize data
		agent_ = Agent();

		// Set speed planner parameters
		speed_planner_.setParameters(min_speed, max_speed, min_distance, max_distance, max_acceleration);

		// Create mavros communication
		mavros_comm_ = std::make_shared<MavrosCommunication>(agent_id_, node_, config_tools_, transform_tools_);

		// Subscribe to status, position and setpoint topics
		agent_.status_sub = topic_tools_->createAgentStatusSubscriber(agent_id_,
			std::bind(&DroneControl::statusCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
		agent_.position_sub = topic_tools_->createAgentPositionSubscriber(agent_id_,
			std::bind(&DroneControl::positionCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
		agent_.setpoint_sub = topic_tools_->createAgentPositionSetpointSubscriber(agent_id_,
			std::bind(&DroneControl::setpointPositionCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

		// Set update timer
		last_update_time_ = RosUtils::now(node_);
		update_timer_ = RosUtils::createTimer(node_, update_rate_,
			std::bind(&DroneControl::update, this), module_cb_group_);
	}

	void DroneControl::onShutdown()
	{
		// Destroy subscribers
		agent_.status_sub.reset();
		agent_.position_sub.reset();
		agent_.setpoint_sub.reset();
		// Destroy update timer
		update_timer_.reset();
	}

	// ════════════════════════════════════════════════════════════════════════════
	// CALLBACKS: Callback functions
	// ════════════════════════════════════════════════════════════════════════════

	void DroneControl::statusCallback(const core::AgentStatusMsg::SharedPtr msg)
	{
		// Update current status
		agent_.status = static_cast<AgentStatus>(msg->status);
		agent_.has_status = true;
	}

	void DroneControl::positionCallback(const core::PointStampedMsg::SharedPtr msg)
	{
		// Update current position
		agent_.position = msg->point;
		agent_.has_position = true;
	}

	void DroneControl::setpointPositionCallback(const core::PointStampedMsg::SharedPtr msg)
	{
		// Update setpoint position
		agent_.setpoint = msg->point;
		agent_.has_setpoint = true;
	}

	// ════════════════════════════════════════════════════════════════════════════
	// UPDATE: Update control
	// ════════════════════════════════════════════════════════════════════════════

	void DroneControl::update()
	{
		// Check if we have a valid status, position and setpoint
		if (!agent_.has_status || !agent_.has_position || !agent_.has_setpoint)
		{
			RCLCPP_WARN(node_->get_logger(), "Drone control: No status, position or setpoint data received for agent %s",
				agent_id_.c_str());
			return;
		}

		// Check if we are in the correct state to move
		if (agent_.status != AgentStatus::TRACKING)
		{
			RCLCPP_WARN(node_->get_logger(), "Drone control: Agent %s is not in the correct state to move",
				agent_id_.c_str());
			return;
		}

		// Compute time step
		auto current_time = RosUtils::now(node_);
		float dt = (current_time - last_update_time_).seconds();
		last_update_time_ = current_time;

		// Limit dt to prevent extreme values after pauses
		dt = std::min(dt, cmd_timeout_);

		switch (control_mode_)
		{
		case ControlMode::POSITION:
			handlePositionControl(dt);
			break;

		case ControlMode::VELOCITY:
			handleVelocityControl(dt);
			break;

		default:
			RCLCPP_ERROR(node_->get_logger(), "Drone control: Invalid control mode for agent %s",
				agent_id_.c_str());
			break;
		}
	}

	void DroneControl::handlePositionControl(const float& dt)
	{
		// Send command to move to goal position
		mavros_comm_->enableOffboard(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		mavros_comm_->setPosition(agent_.setpoint.x, agent_.setpoint.y, agent_.setpoint.z);
	}

	void DroneControl::handleVelocityControl(const float& dt)
	{
		// TODO: Implement velocity control
	}

} // namespace flychams::control