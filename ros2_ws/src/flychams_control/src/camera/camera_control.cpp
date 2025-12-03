#include "flychams_control/camera/camera_control.hpp"

using namespace flychams::core;

namespace flychams::control
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void CameraControl::onInit()
	{
		// Get parameters from parameter server
		// Get update rate
		update_rate_ = RosUtils::getParameterOr<float>(node_, "camera_control.control_update_rate", 20.0f);

		// Initialize data
		agent_ = Agent();

		// Subscribe to status and head setpoints topics
		agent_.status_sub = topic_tools_->createAgentStatusSubscriber(agent_id_,
			std::bind(&CameraControl::statusCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
		agent_.setpoints_sub = topic_tools_->createAgentObservationSetpointsSubscriber(agent_id_,
			std::bind(&CameraControl::setpointsCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

		// Set update timer
		update_timer_ = RosUtils::createTimer(node_, update_rate_,
			std::bind(&CameraControl::update, this), module_cb_group_);
	}

	void CameraControl::onShutdown()
	{
		// Destroy subscribers
		agent_.status_sub.reset();
		agent_.setpoints_sub.reset();
		// Destroy update timer
		update_timer_.reset();
	}

	// ════════════════════════════════════════════════════════════════════════════
	// CALLBACKS: Callback functions
	// ════════════════════════════════════════════════════════════════════════════

	void CameraControl::statusCallback(const core::AgentStatusMsg::SharedPtr msg)
	{
		// Update current status
		agent_.status = static_cast<AgentStatus>(msg->status);
		agent_.has_status = true;
	}

	void CameraControl::setpointsCallback(const core::AgentObservationSetpointsMsg::SharedPtr msg)
	{
		// Update observation setpoints
		agent_.setpoints = *msg;
		agent_.has_setpoints = true;
	}

	// ════════════════════════════════════════════════════════════════════════════
	// UPDATE: Update cameras
	// ════════════════════════════════════════════════════════════════════════════

	void CameraControl::update()
	{
		// Check if we have a valid status and setpoints
		if (!agent_.has_status || !agent_.has_setpoints)
		{
			RCLCPP_WARN(node_->get_logger(), "Camera control: No status or setpoints data received for agent %s",
				agent_id_.c_str());
			return;
		}

		// Check if we are in the correct state to move
		if (agent_.status != AgentStatus::MISSION)
		{
			RCLCPP_WARN(node_->get_logger(), "Camera control: Agent %s is not in the correct state to control cameras",
				agent_id_.c_str());
			return;
		}

		// Iterate over all cameras (including the central camera) to fill vectors
		const int& n_o = agent_.setpoints.n_o;
		const int& n_commands = agent_.setpoints.n_c + 1; // +1 for the central camera
		std::vector<ID> unit_ids(n_commands);
		std::vector<QuaternionMsg> unit_quats(n_commands);
		std::vector<float> unit_fovs(n_commands);
		for (int i = 0, j = 0; i < n_o; i++)
		{
			// Filter out units that are not cameras
			if (agent_.setpoints.types[i] != 1)
			{
				continue;
			}

			// Get camera ID
			unit_ids[j] = agent_.setpoints.ids[i];

			// Get camera configuration
			const auto& camera_config = config_tools_->getCamera(agent_id_, unit_ids[j]);

			// Get camera quaternion
			const auto& rotation = agent_.setpoints.rotations[i];
			Vector3r rpy_vec = Vector3r(rotation.x, rotation.y, rotation.z);
			RosUtils::toMsg(MathUtils::eulerToQuaternion(rpy_vec), unit_quats[j]);

			// Calculate camera FOV
			const float& focal = agent_.setpoints.zoom_factors[i];
			const float& sensor_width = camera_config.sensor_size(0);
			unit_fovs[j] = MathUtils::computeFov(focal, sensor_width);

			// Increment index
			j++;
		}

		// Send commands to cameras
		framework_tools_->setGimbalOrientations(agent_id_, unit_ids, unit_quats);
		framework_tools_->setCameraFovs(agent_id_, unit_ids, unit_fovs);
	}

} // namespace flychams::control