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

		// Get flight parameters
		takeoff_altitude_ = RosUtils::getParameterOr<float>(node_, "drone_control.takeoff_altitude", 1.5f);

		// Get space constraints
		const auto& config_ptr = config_tools_->getConfig();
		const auto& agent_ptr = config_tools_->getAgent(agent_id_);
		float min_horizontal = config_ptr->horizontal_constraint(0);
		float max_horizontal = config_ptr->horizontal_constraint(1);
		float min_vertical = config_ptr->vertical_constraint(0);
		float max_vertical = std::min(config_ptr->vertical_constraint(1), agent_ptr->max_altitude);
		flying_box_.min_x = min_horizontal;
		flying_box_.max_x = max_horizontal;
		flying_box_.min_y = min_horizontal;
		flying_box_.max_y = max_horizontal;
		flying_box_.min_z = min_vertical;
		flying_box_.max_z = max_vertical;

		// Initialize data
		agent_ = Agent();

		// Initialize command counter
		command_counter_ = 0;

		// Create mavros communication
		mavros_comm_ = std::make_shared<MavrosCommunication>(agent_id_, node_);

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
		// Destroy mavros communication
		mavros_comm_.reset();
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
		// Check if we have a valid status and position
		if (!agent_.has_status || !agent_.has_position)
		{
			RCLCPP_WARN(node_->get_logger(), "Drone control: No status or position data received for agent %s",
				agent_id_.c_str());
			return;
		}

		// Compute time step
		auto current_time = RosUtils::now(node_);
		float dt = (current_time - last_update_time_).seconds();
		last_update_time_ = current_time;

		// Initialize success variable
		bool success = true;

		// Proceed based on the status of the agent
		switch (agent_.status)
		{
		case AgentStatus::IDLE:
			success &= requestTakeoff();
			if (command_counter_ > 10)
			{
				success &= requestOffboard();
				success &= requestArm();
			}
			if (!success)
			{
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to set agent %s in idle mode",
					agent_id_.c_str());
				return;
			}
			command_counter_++;
			break;
		case AgentStatus::TAKEOFF:
			success &= requestTakeoff();
			if (!success)
			{
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to takeoff agent %s in takeoff mode",
					agent_id_.c_str());
				return;
			}
			command_counter_ = 0;
			break;
		case AgentStatus::MISSION:
			switch (control_mode_)
			{
			case ControlMode::POSITION:
				// Check if we have a valid setpoint
				if (agent_.has_setpoint)
				{
					// We have a valid setpoint
					// Check if the setpoint is inside the flying box
					if (isInsideFlyingBox(agent_.setpoint))
					{
						// The setpoint is inside the flying box, so we move to it
						success &= requestSetpoint();
					}
					else
					{
						// The setpoint is outside the flying box, so we hover
						success &= requestHover();
					}
				}
				else
				{
					// We don't have a valid setpoint, so we hover
					success &= requestHover();
				}
				break;

			case ControlMode::VELOCITY:
				// TODO: Implement velocity control
				break;
			}

			if (!success)
			{
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to control agent %s in mission mode",
					agent_id_.c_str());
				return;
			}
			command_counter_ = 0;
			break;
		case AgentStatus::LAND:
			success &= requestLand();
			if (!success)
			{
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to land agent %s in landing mode",
					agent_id_.c_str());
				return;
			}
			command_counter_ = 0;
			break;
		case AgentStatus::ERROR:
			success &= requestLand();
			if (!success)
			{
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to land agent %s in error mode",
					agent_id_.c_str());
				return;
			}
			command_counter_ = 0;
			break;
		default:
			// The agent is in an unknown status, so we land
			success &= requestLand();
			if (!success)
			{
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to land agent %s in unknown mode",
					agent_id_.c_str());
				return;
			}
			command_counter_ = 0;
			break;
		}
	}

	// ════════════════════════════════════════════════════════════════════════════
	// REQUESTS: Request methods to control the drone
	// ════════════════════════════════════════════════════════════════════════════

	bool DroneControl::requestOffboard()
	{
		return mavros_comm_->enableOffboard(true);
	}

	bool DroneControl::requestDisarm()
	{
		if (agent_.status == AgentStatus::IDLE || agent_.status == AgentStatus::ERROR)
			return mavros_comm_->armDisarm(false);
		else
			return false;
	}

	bool DroneControl::requestArm()
	{
		if (agent_.status == AgentStatus::IDLE)
			return mavros_comm_->armDisarm(true);
		else
			return false;
	}

	bool DroneControl::requestTakeoff()
	{
		if (agent_.status == AgentStatus::IDLE || agent_.status == AgentStatus::TAKEOFF)
		{
			mavros_comm_->setPosition(agent_.position.x, agent_.position.y, takeoff_altitude_);
			return true;
		}
		else
			return false;
	}

	bool DroneControl::requestHover()
	{
		if (agent_.status == AgentStatus::TAKEOFF || agent_.status == AgentStatus::MISSION)
		{
			mavros_comm_->setPosition(agent_.position.x, agent_.position.y, agent_.position.z);
			return true;
		}
		else
			return false;
	}

	bool DroneControl::requestSetpoint()
	{
		if (agent_.status == AgentStatus::MISSION)
		{
			mavros_comm_->setPosition(agent_.setpoint.x, agent_.setpoint.y, agent_.setpoint.z);
			RCLCPP_INFO(node_->get_logger(), "Drone control: Setpoint sent to agent %s",
				agent_id_.c_str());
			RCLCPP_INFO(node_->get_logger(), "Drone control: Setpoint: %f, %f, %f",
				agent_.setpoint.x, agent_.setpoint.y, agent_.setpoint.z);
			return true;
		}
		else
			return false;
	}

	bool DroneControl::requestLand()
	{
		if (agent_.status == AgentStatus::MISSION || agent_.status == AgentStatus::ERROR)
			return mavros_comm_->land();
		else
			return false;
	}

	// ════════════════════════════════════════════════════════════════════════════
	// HELPER METHODS
	// ════════════════════════════════════════════════════════════════════════════

	bool DroneControl::isInsideFlyingBox(const core::PointMsg& point)
	{
		return point.x >= flying_box_.min_x && point.x <= flying_box_.max_x &&
			point.y >= flying_box_.min_y && point.y <= flying_box_.max_y &&
			point.z >= flying_box_.min_z && point.z <= flying_box_.max_z;
	}

} // namespace flychams::control