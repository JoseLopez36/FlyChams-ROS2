#include "flychams_bringup/frames/frames_manager.hpp"

using namespace flychams::core;

namespace flychams::bringup
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void FramesManager::onInit()
	{
		// Subscribe to home position
        std::string home_topic = "/mavros/" + agent_id_ + "/home_position/home";
        home_sub_ = node_->create_subscription<mavros_msgs::msg::HomePosition>(
            home_topic, 
            rclcpp::QoS(10).transient_local(),
            std::bind(&FramesManager::homeCallback, this, std::placeholders::_1),
            sub_options_with_module_cb_group_
        );

		// Subscribe to local odometry
		std::string odom_topic = "/mavros/" + agent_id_ + "/local_position/odom";
		rclcpp::QoS odom_qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();
		odom_sub_ = node_->create_subscription<OdometryMsg>(
			odom_topic,
			odom_qos,
			std::bind(&FramesManager::odomCallback, this, std::placeholders::_1),
			sub_options_with_module_cb_group_
		);

		// Set update timer
		update_timer_ = RosUtils::createTimer(node_, 10.0f,
			std::bind(&FramesManager::update, this), module_cb_group_);
	}

	void FramesManager::onShutdown()
	{
		// Reset subscribers
        home_sub_.reset();
		odom_sub_.reset();
	}

	// ════════════════════════════════════════════════════════════════════════════
	// CALLBACKS: Callback functions
	// ════════════════════════════════════════════════════════════════════════════

    void FramesManager::homeCallback(const mavros_msgs::msg::HomePosition::SharedPtr msg)
    {
        if (frames_setup_flag_)
            return;
        
        RCLCPP_INFO(node_->get_logger(), "Received home position for agent %s", agent_id_.c_str());

		// Setup frames
		setupAgentFrames(msg->position, msg->orientation);

        // Mark as setup
        frames_setup_flag_ = true;
    }

	void FramesManager::odomCallback(const OdometryMsg::SharedPtr msg)
	{
		// Update latest odometry
		latest_odom_ = *msg;
		has_odom_ = true;
	}

	// ════════════════════════════════════════════════════════════════════════════
	// FRAMES MANAGEMENT: Frames management functions
	// ════════════════════════════════════════════════════════════════════════════

	void FramesManager::setupAgentFrames(const geometry_msgs::msg::Point& home_position, const geometry_msgs::msg::Quaternion& home_orientation)
	{
		RCLCPP_INFO(node_->get_logger(), "Setting up frames for agent %s", agent_id_.c_str());

        // Get world frame
        std::string world_frame = transform_tools_->getGlobalFrame();

        // Get agent local frame
        std::string agent_local_frame = transform_tools_->getAgentLocalFrame(agent_id_);

		// Use agent initial position as home position
		const core::Vector3r& initial_position = config_tools_->getAgent(agent_id_)->position;

        // Calculate translation from world to agent local
        // Vector3r world_to_map_translation = Vector3r(home_position.x, home_position.y, home_position.z);
		Vector3r world_to_local_translation = initial_position;

        // Orientation from message
        // Quaternionr q(
        //     static_cast<float>(home_orientation.w),
        //     static_cast<float>(home_orientation.x),
        //     static_cast<float>(home_orientation.y),
        //     static_cast<float>(home_orientation.z)
        // );
        Quaternionr q(
            1.0f,
            0.0f,
            0.0f,
            0.0f
        );
        Matrix3r world_to_local_rotation = q.toRotationMatrix();

        // Create transform matrix for world -> agent map
        Matrix4r world_to_local = Matrix4r::Identity();
        world_to_local.block<3, 3>(0, 0) = world_to_local_rotation;
        world_to_local.block<3, 1>(0, 3) = world_to_local_translation;

        // Broadcast world -> agent map (static)
        transform_tools_->broadcastStaticTransform(world_frame, agent_local_frame, world_to_local);
        RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", world_frame.c_str(), agent_local_frame.c_str());

		// Setup camera frames
		setupCameraFrames();
	}

	void FramesManager::setupCameraFrames()
	{
		// Get agent body frame
		std::string agent_body_frame = transform_tools_->getAgentBodyFrame(agent_id_);

		// Get multi-camera set for this agent
		const auto& multi_camera_set = config_tools_->getMultiCameraSet(agent_id_);

		// Iterate through all cameras
		for (const auto& [camera_id, multi_camera_config] : multi_camera_set)
		{
			// Get camera position and orientation from config
			const Vector3r& camera_position = multi_camera_config->position;
			const Vector3r& camera_orientation_rpy = multi_camera_config->orientation;

			// Convert RPY to quaternion
			Quaternionr camera_orientation = MathUtils::eulerToQuaternion(camera_orientation_rpy);

			// Get camera body and optical frame names
			std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);
			std::string camera_optical_frame = transform_tools_->getCameraOpticalFrame(agent_id_, camera_id);

			// Create transform matrix from agent body to camera body
			Matrix4r body_to_camera_body = Matrix4r::Identity();
			body_to_camera_body.block<3, 3>(0, 0) = camera_orientation.toRotationMatrix();
			body_to_camera_body.block<3, 1>(0, 3) = camera_position;

			// Broadcast agent body -> camera body (static, initial position)
			transform_tools_->broadcastStaticTransform(agent_body_frame, camera_body_frame, body_to_camera_body);
			RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", agent_body_frame.c_str(), camera_body_frame.c_str());

			// Create camera body to optical transform (standard optical frame rotation)
			// Optical frame: 90 deg rotation around X and Y axes
			Quaternionr optical_rotation(0.5f, -0.5f, 0.5f, -0.5f); // w, x, y, z
			Matrix4r camera_body_to_optical = Matrix4r::Identity();
			camera_body_to_optical.block<3, 3>(0, 0) = optical_rotation.toRotationMatrix();

			// Broadcast camera body -> camera optical (static)
			transform_tools_->broadcastStaticTransform(camera_body_frame, camera_optical_frame, camera_body_to_optical);
			RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", camera_body_frame.c_str(), camera_optical_frame.c_str());
		}
	}

	// ════════════════════════════════════════════════════════════════════════════
	// UPDATE: Update frames
	// ════════════════════════════════════════════════════════════════════════════

	void FramesManager::update()
	{
		// Only update if frames are set up and we have odometry
		if (!frames_setup_flag_ || !has_odom_)
			return;

		// Get agent local frame and body frame
		std::string agent_local_frame = transform_tools_->getAgentLocalFrame(agent_id_);
		std::string agent_body_frame = transform_tools_->getAgentBodyFrame(agent_id_);

		// Get pose from odometry (odometry is in agent map frame)
		const PoseMsg& pose = latest_odom_.pose.pose;

		// Convert pose position to Vector3r
		Vector3r translation = RosUtils::fromMsg(pose.position);

		// Convert pose orientation to Quaternionr
		Quaternionr orientation = RosUtils::fromMsg(pose.orientation);

		// Create transform matrix from pose
		Matrix4r local_to_body = Matrix4r::Identity();
		local_to_body.block<3, 3>(0, 0) = orientation.toRotationMatrix();
		local_to_body.block<3, 1>(0, 3) = translation;

		// Broadcast agent local -> agent body transform
		transform_tools_->broadcastTransform(agent_local_frame, agent_body_frame, local_to_body);
	}

} // namespace flychams::bringup
