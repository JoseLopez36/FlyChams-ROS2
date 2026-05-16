#include "flychams_agent/drone/drone_control.hpp"

using namespace flychams::common;

namespace flychams::agent
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void DroneControl::onModuleInit()
	{
		// Get parameters from parameter server
		// Get update rate
		update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);
		// Get control mode
		control_mode_ = static_cast<ControlMode>(node_->getParameterOr<uint8_t>("control_mode", 0));
		// Get flight parameters
		takeoff_altitude_ = node_->getParameterOr<float>("takeoff_altitude", 1.5f);

		// Get space constraints
		const auto& config_ptr = node_->getSettings()->getConfig();
		const auto& agent_ptr = node_->getSettings()->getAgent(agent_id_);
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
		agent_.status_sub = node_->createAgentStatusSubscriber(agent_id_,
			std::bind(&DroneControl::statusCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());
		agent_.local_position_sub = node_->createAgentLocalPositionSubscriber(agent_id_,
			std::bind(&DroneControl::localPositionCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());
		agent_.setpoint_sub = node_->createAgentPositionSetpointSubscriber(agent_id_,
			std::bind(&DroneControl::setpointPositionCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

		// Set update timer
		last_update_time_ = node_->now();
		update_timer_ = node_->createTimer(update_rate_, std::bind(&DroneControl::update, this));
	}

	void DroneControl::onModuleShutdown()
	{
		// Destroy subscribers
		agent_.status_sub.reset();
		agent_.local_position_sub.reset();
		agent_.setpoint_sub.reset();
		// Destroy mavros communication
		mavros_comm_.reset();
		// Destroy update timer
		update_timer_.reset();
	}

	// ════════════════════════════════════════════════════════════════════════════
	// CALLBACKS: Callback functions
	// ════════════════════════════════════════════════════════════════════════════

	void DroneControl::statusCallback(const AgentStatusMsg::SharedPtr msg)
	{
		// Update current status
		agent_.status = static_cast<AgentStatus>(msg->status);
		agent_.has_status = true;
	}

	void DroneControl::localPositionCallback(const PointStampedMsg::SharedPtr msg)
	{
		// Update current local position
		agent_.local_position = *msg;
		agent_.has_local_position = true;
	}

	void DroneControl::setpointPositionCallback(const PointStampedMsg::SharedPtr msg)
	{
		// Update setpoint position
		agent_.setpoint = *msg;
		agent_.has_setpoint = true;
	}

	// ════════════════════════════════════════════════════════════════════════════
	// UPDATE: Update control
	// ════════════════════════════════════════════════════════════════════════════

	void DroneControl::update()
	{
		// Check if we have a valid status and position
		if (!agent_.has_status || !agent_.has_local_position)
		{
			RCLCPP_WARN(node_->get_logger(), "Drone control: No status or local position data received for agent %s",
				agent_id_.c_str());
			return;
		}

		// Compute time step
		auto current_time = node_->now();
		float dt = (current_time - last_update_time_).seconds();
		last_update_time_ = current_time;
		(void)dt;

		bool success = true;

		// Mission-level limiting: PAUSED → hover, ABORTED → land
		if (node_->isMissionAborted())
		{
			success &= requestLand();
			if (!success)
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to land agent %s on mission ABORT",
					agent_id_.c_str());
			command_counter_ = 0;
			return;
		}

		if (node_->isMissionPaused())
		{
			success &= requestHover();
			if (!success)
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to hover agent %s on mission PAUSE",
					agent_id_.c_str());
			command_counter_ = 0;
			return;
		}

		// Proceed based on the 3-state agent status
		switch (agent_.status)
		{
		case AgentStatus::IDLE:
			// Agent is on the ground and disarmed: prepare for takeoff
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
		case AgentStatus::ACTIVE:
			// Agent is armed and flying: execute mission setpoints
			switch (control_mode_)
			{
			case ControlMode::POSITION:
				if (agent_.has_setpoint)
				{
					if (isInsideFlyingBox(agent_.setpoint.point))
						success &= requestSetpoint();
					else
						success &= requestHover();
				}
				else
				{
					success &= requestHover();
				}
				break;
			case ControlMode::VELOCITY:
				// TODO: Implement velocity control
				break;
			}
			if (!success)
			{
				RCLCPP_ERROR(node_->get_logger(), "Drone control: Failed to control agent %s in active mode",
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
		if (agent_.status == AgentStatus::IDLE)
		{
			mavros_comm_->setLocalPosition(0.0f, 0.0f, takeoff_altitude_);
			return true;
		}
		else
			return false;
	}

	bool DroneControl::requestHover()
	{
		if (agent_.status == AgentStatus::ACTIVE)
		{
			mavros_comm_->setLocalPosition(agent_.local_position.point.x, agent_.local_position.point.y, agent_.local_position.point.z);
			return true;
		}
		else
			return false;
	}

	bool DroneControl::requestSetpoint()
	{
		if (agent_.status == AgentStatus::ACTIVE)
		{
			const std::string& local_frame = node_->getAgentLocalFrame(agent_id_);
			const PointStampedMsg local_setpoint = node_->transformPoint(agent_.setpoint, local_frame);

			mavros_comm_->setLocalPosition(local_setpoint.point.x, local_setpoint.point.y, local_setpoint.point.z);
			RCLCPP_DEBUG(node_->get_logger(), "Drone control: Setpoint sent to agent %s",
				agent_id_.c_str());
			RCLCPP_DEBUG(node_->get_logger(), "Drone control: Setpoint: %f, %f, %f",
				agent_.setpoint.point.x, agent_.setpoint.point.y, agent_.setpoint.point.z);
			return true;
		}
		else
			return false;
	}

	bool DroneControl::requestLand()
	{
		if (agent_.status == AgentStatus::ACTIVE || agent_.status == AgentStatus::ERROR)
			return mavros_comm_->land();
		else
			return false;
	}

	// ════════════════════════════════════════════════════════════════════════════
	// HELPER METHODS
	// ════════════════════════════════════════════════════════════════════════════

	bool DroneControl::isInsideFlyingBox(const PointMsg& point)
	{
		return point.x >= flying_box_.min_x && point.x <= flying_box_.max_x &&
			point.y >= flying_box_.min_y && point.y <= flying_box_.max_y &&
			point.z >= flying_box_.min_z && point.z <= flying_box_.max_z;
	}

} // namespace flychams::agent