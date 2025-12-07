#include "flychams_bringup/frames/frames_manager.hpp"

using namespace flychams::core;

namespace flychams::bringup
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void FramesManager::onInit()
	{
		// Publish agent local frame
		transform_tools_->broadcastStaticTransform(transform_tools_->getGlobalFrame(), transform_tools_->getAgentLocalFrame(agent_id_), Matrix4r::Identity());
		RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", transform_tools_->getGlobalFrame().c_str(), transform_tools_->getAgentLocalFrame(agent_id_).c_str());
		
		// Setup camera frames
		setupCameraFrames();
	}

	void FramesManager::onShutdown()
	{
		// Nothing to do
	}

	// ════════════════════════════════════════════════════════════════════════════
	// FRAMES MANAGEMENT: Frames management functions
	// ════════════════════════════════════════════════════════════════════════════

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

} // namespace flychams::bringup
