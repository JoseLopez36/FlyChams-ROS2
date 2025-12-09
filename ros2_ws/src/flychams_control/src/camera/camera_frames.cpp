#include "flychams_control/camera/camera_frames.hpp"

using namespace flychams::core;

namespace flychams::control
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "camera_frames.update_rate", 5.0f);

        // Initialize data
        agent_ = Agent();

        // Get multi camera set
        auto multi_camera_set = config_tools_->getMultiCameraSet(agent_id_);

        // Iterate through all cameras to create initial frames
        for (const auto& [camera_id, camera_config_ptr] : multi_camera_set)
        {
            createCameraBodyFrame(camera_id, camera_config_ptr);
            createCameraOpticalFrame(camera_id, camera_config_ptr);
        }

        // Subscribe to topics
        agent_.setpoints_sub = topic_tools_->createAgentObservationSetpointsSubscriber(agent_id_,
            std::bind(&CameraFrames::setpointsCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
    }

    void CameraFrames::onShutdown()
    {
        // Destroy subscriber
        agent_.setpoints_sub.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::setpointsCallback(const core::AgentObservationSetpointsMsg::SharedPtr msg)
    {
        // Update observation setpoints
        agent_.setpoints = *msg;
        agent_.has_setpoints = true;

        // Iterate over all cameras in setpoints
        const int& n_o = agent_.setpoints.n_o;

        for (int i = 0; i < n_o; i++)
        {
            // Filter out units that are not cameras
            if (agent_.setpoints.types[i] != 1)
            {
                continue;
            }

            // Get camera ID
            ID camera_id = agent_.setpoints.ids[i];

            // Get camera configuration
            const auto& camera_config_ptr = config_tools_->getMultiCamera(agent_id_, camera_id);

            // Get camera quaternion from command (rotation relative to body or gimbal frame)
            // Note: The rotation in setpoints is Euler angles (RPY)
            const auto& rotation = agent_.setpoints.rotations[i];
            Vector3r rpy_vec = Vector3r(rotation.x, rotation.y, rotation.z);
            QuaternionMsg orientation_msg;
            RosUtils::toMsg(MathUtils::eulerToQuaternion(rpy_vec), orientation_msg);

            // Get camera position from config (static relative to body)
            PointMsg position_msg;
            position_msg.x = camera_config_ptr->position.x();
            position_msg.y = camera_config_ptr->position.y();
            position_msg.z = camera_config_ptr->position.z();

            // Update frame
            updateCameraBodyFrame(camera_id, position_msg, orientation_msg);
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // FRAMES CREATION: Frames creation
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::createCameraBodyFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr)
    {
        // Get frames
        std::string body_frame = transform_tools_->getAgentBodyFrame(agent_id_);
        std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);

        // Get camera position and orientation from config
        const Vector3r& camera_position = camera_config_ptr->position;
        const Vector3r& camera_orientation_rpy = camera_config_ptr->orientation;

        // Convert RPY to quaternion
        Quaternionr camera_orientation = MathUtils::eulerToQuaternion(camera_orientation_rpy);

        // Create transform matrix from agent body to camera body
        Matrix4r body_to_camera_body = Matrix4r::Identity();
        body_to_camera_body.block<3, 3>(0, 0) = camera_orientation.toRotationMatrix();
        body_to_camera_body.block<3, 1>(0, 3) = camera_position;

        // Broadcast agent body -> camera body
        transform_tools_->broadcastTransform(body_frame, camera_body_frame, body_to_camera_body);
        RCLCPP_INFO(node_->get_logger(), "Published transform: %s -> %s", body_frame.c_str(), camera_body_frame.c_str());
    }

    void CameraFrames::createCameraOpticalFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr)
    {
        // Get frames
        std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);
        std::string camera_optical_frame = transform_tools_->getCameraOpticalFrame(agent_id_, camera_id);

        // Initialize camera optical at camera body origin
        Matrix4r camera_body_to_camera_optical = Matrix4r::Identity();

        // Broadcast camera body -> camera optical (static)
        transform_tools_->broadcastStaticTransform(camera_body_frame, camera_optical_frame, camera_body_to_camera_optical);
        RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", camera_body_frame.c_str(), camera_optical_frame.c_str());
    }

    // ════════════════════════════════════════════════════════════════════════════
    // FRAMES UPDATE: Frames update
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::updateCameraBodyFrame(const core::ID camera_id, const core::PointMsg& position, const core::QuaternionMsg& orientation)
    {
        // Get frames
        std::string body_frame = transform_tools_->getAgentBodyFrame(agent_id_);
        std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);

        // Get body to camera body transformation
        Matrix4r body_to_camera_body = Matrix4r::Identity();

        // Set position
        body_to_camera_body(0, 3) = position.x;
        body_to_camera_body(1, 3) = position.y;
        body_to_camera_body(2, 3) = position.z;

        // Set orientation
        Quaternionr orientation_quat = RosUtils::fromMsg(orientation);
        body_to_camera_body.block<3, 3>(0, 0) = MathUtils::quaternionToRotationMatrix(orientation_quat);

        // Broadcast agent body -> camera body
        transform_tools_->broadcastTransform(body_frame, camera_body_frame, body_to_camera_body);
    }

} // namespace flychams::control

