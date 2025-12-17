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

        // Initialize communication
        camera_communication_ = std::make_shared<CameraCommunication>(agent_id_, node_);

        // Get multi camera set
        auto multi_camera_set = settings_tools_->getMultiCameraSet(agent_id_);

        // Iterate through all cameras to create initial frames
        for (const auto& [camera_id, camera_config_ptr] : multi_camera_set)
        {
            // Get camera position from config (static relative to body)
            PointMsg position_msg;
            position_msg.x = camera_config_ptr->position.x();
            position_msg.y = camera_config_ptr->position.y();
            position_msg.z = camera_config_ptr->position.z();

            // Get camera quaternion from config (static relative to body)
            QuaternionMsg orientation_msg;
            RosUtils::toMsg(TfUtils::eulerToQuat(camera_config_ptr->orientation), orientation_msg);

            // Create frames
            updateCameraBodyFrame(camera_id, position_msg, orientation_msg);
            createCameraOpticalFrame(camera_id);
        }

        // Subscribe to topics
        agent_.camera_orientation_sub = camera_communication_->subscribeCameraOrientation(
            std::bind(&CameraFrames::cameraOrientationCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
    }

    void CameraFrames::onShutdown()
    {
        // Destroy subscriber
        agent_.camera_orientation_sub.reset();
        // Shutdown communication
        camera_communication_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::cameraOrientationCallback(const airsim_interfaces::msg::CameraOrientation::SharedPtr msg)
    {
        // Iterate through all cameras in the message
        for (size_t i = 0; i < msg->camera_names.size(); i++)
        {
            // Get camera ID and orientation
            ID camera_id = msg->camera_names[i];
            const auto& orientation_msg = msg->orientations[i];

            // Get camera configuration
            const auto& camera_config_ptr = settings_tools_->getMultiCamera(agent_id_, camera_id);

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

    void CameraFrames::createCameraOpticalFrame(const core::ID camera_id)
    {
        // Get frames
        std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);
        std::string camera_optical_frame = transform_tools_->getCameraOpticalFrame(agent_id_, camera_id);

        // Initialize camera optical at camera body origin with optical frame rotation
        Matrix4r camera_body_to_camera_optical = Matrix4r::Identity();

        // Apply optical frame rotation: rotate from camera body to optical frame
        // This rotation aligns the camera coordinate system with the optical frame convention
        TransformMsg optical_tf = TransformMsg();
        auto optical_quat = Quaternionr(optical_tf.rotation.w, optical_tf.rotation.x, optical_tf.rotation.y, optical_tf.rotation.z);
        optical_quat *= Quaternionr(0.5, -0.5, 0.5, -0.5);
        camera_body_to_camera_optical.block<3, 3>(0, 0) = TfUtils::quatToMatrix(optical_quat);

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
        std::string world_frame = transform_tools_->getGlobalFrame();
        std::string body_frame = transform_tools_->getAgentBodyFrame(agent_id_);
        std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);

        // Lookup body pose in world using TF
        PoseStampedMsg pose;
        pose.header = RosUtils::createHeader(node_, body_frame);
        pose.pose.position = PointMsg();
        pose.pose.orientation = QuaternionMsg();
        pose.pose.orientation.w = 1.0;
        const PoseStampedMsg wTb = transform_tools_->transformPose(pose, world_frame);

        // Extract world to body pose
        const Vector3r wPb(
            wTb.pose.position.x,
            wTb.pose.position.y,
            wTb.pose.position.z);
        const Quaternionr wQb = RosUtils::fromMsg(wTb.pose.orientation);
        const Matrix3r wRb = TfUtils::quatToMatrix(wQb);

        // Camera mounting offset in body frame
        const Vector3r bPc(position.x, position.y, position.z);

        // Get world to camera body position and orientation
        const Vector3r wPc = wPb + (wRb * bPc);
        const Quaternionr wQc = RosUtils::fromMsg(orientation);

        // Build world to camera body transform
        Matrix4r wTc = Matrix4r::Identity();
        wTc(0, 3) = wPc.x();
        wTc(1, 3) = wPc.y();
        wTc(2, 3) = wPc.z();
        wTc.block<3, 3>(0, 0) = TfUtils::quatToMatrix(wQc);

        // Broadcast world -> camera body (dynamic)
        transform_tools_->broadcastTransform(world_frame, camera_body_frame, wTc);
    }

} // namespace flychams::control

