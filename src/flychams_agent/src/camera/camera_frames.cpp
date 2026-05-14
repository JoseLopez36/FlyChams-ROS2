#include "flychams_agent/camera/camera_frames.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "update_rate", 30.0f);

        // Initialize data
        agent_ = Agent();

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

            // Create initial camera body frame with identity orientation
            QuaternionMsg orientation_msg;
            orientation_msg.x = 0.0;
            orientation_msg.y = 0.0;
            orientation_msg.z = 0.0;
            orientation_msg.w = 1.0;

            // Create frames
            updateCameraBodyFrame(camera_id, position_msg, orientation_msg);
            createCameraOpticalFrame(camera_id);
        }

        // Subscribe to agent observation setpoints
        agent_.observation_setpoints_sub = topic_tools_->createAgentObservationSetpointsSubscriber(agent_id_,
            std::bind(&CameraFrames::observationSetpointsCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Set update timer
        update_timer_ = rclcpp::create_timer(node_, 
            node_->get_clock(), 
            std::chrono::duration<float>(1.0f / update_rate_), 
            std::bind(&CameraFrames::update, this), 
            module_cb_group_);
    }

    void CameraFrames::onShutdown()
    {
        // Destroy subscriber
        agent_.observation_setpoints_sub.reset();
        // Destroy update timer
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::observationSetpointsCallback(const core::AgentObservationSetpointsMsg::SharedPtr msg)
    {
        // Update observation setpoints
        agent_.observation_setpoints = *msg;
        agent_.has_observation_setpoints = true;
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
        camera_body_to_camera_optical.block<3, 3>(0, 0) = MathUtils::quatToMatrix(optical_quat);

        // Broadcast camera body -> camera optical (static)
        transform_tools_->broadcastStaticTransform(camera_body_frame, camera_optical_frame, camera_body_to_camera_optical);
        RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", camera_body_frame.c_str(), camera_optical_frame.c_str());
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE: Update frames
    // ════════════════════════════════════════════════════════════════════════════

    void CameraFrames::update()
    {
        // Check if we have received observation setpoints
        if (!agent_.has_observation_setpoints)
        {
            return;
        }

        // Iterate through all observation units
        for (int i = 0; i < agent_.observation_setpoints.n_o; ++i)
        {
            // Filter out units that are not cameras
            if (agent_.observation_setpoints.types[i] != static_cast<uint8_t>(core::ObservationType::Camera))
            {
                continue;
            }

            // Get camera ID
            const ID& camera_id = agent_.observation_setpoints.ids[i];

            // Get camera configuration
            const auto& camera_config_ptr = settings_tools_->getMultiCamera(agent_id_, camera_id);

            // Get camera position from config (static relative to body)
            PointMsg position_msg;
            position_msg.x = camera_config_ptr->position.x();
            position_msg.y = camera_config_ptr->position.y();
            position_msg.z = camera_config_ptr->position.z();

            // Convert rotation (RPY) to quaternion
            const auto& rotation = agent_.observation_setpoints.rotations[i];
            Vector3r rpy_vec(rotation.x, rotation.y, rotation.z);

            // Eigen Euler to Quaternion conversion (Z-Y-X order: yaw-pitch-roll)
            Quaternionr quat =
                Eigen::AngleAxisf(rpy_vec.z(), Vector3r::UnitZ()) *
                Eigen::AngleAxisf(rpy_vec.y(), Vector3r::UnitY()) *
                Eigen::AngleAxisf(rpy_vec.x(), Vector3r::UnitX());

            QuaternionMsg orientation_msg;
            RosUtils::toMsg(quat, orientation_msg);

            // Update frame
            updateCameraBodyFrame(camera_id, position_msg, orientation_msg);
        }
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
        const Matrix3r wRb = MathUtils::quatToMatrix(wQb);

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
        wTc.block<3, 3>(0, 0) = MathUtils::quatToMatrix(wQc);

        // Broadcast world -> camera body (dynamic)
        transform_tools_->broadcastTransform(world_frame, camera_body_frame, wTc);
    }

} // namespace flychams::agent