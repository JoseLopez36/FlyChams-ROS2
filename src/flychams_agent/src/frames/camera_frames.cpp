#include "flychams_agent/frames/camera_frames.hpp"

using namespace flychams::common;

using namespace flychams::agent;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void CameraFrames::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate
    update_rate_ = node_->getParameterOr<float>("update_rate", 30.0f);

    // Initialize data
    agent_ = Agent();

    // Get multi camera set
    auto multi_camera_set = node_->getSettings()->getMultiCameraSet(agent_id_);

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
    agent_.observation_setpoints_sub = node_->createObservationSetpointsSubscriber(agent_id_,
        std::bind(&CameraFrames::observationSetpointsCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&CameraFrames::update, this));
}

void CameraFrames::onModuleShutdown()
{
    // Destroy subscriber
    agent_.observation_setpoints_sub.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void CameraFrames::observationSetpointsCallback(const ObservationSetpointsMsg::SharedPtr msg)
{
    // Update observation setpoints
    agent_.observation_setpoints = *msg;
    agent_.has_observation_setpoints = true;
}

// ════════════════════════════════════════════════════════════════════════════
// FRAMES CREATION: Frames creation
// ════════════════════════════════════════════════════════════════════════════

void CameraFrames::createCameraOpticalFrame(const ID camera_id)
{
    // Get frames
    std::string camera_body_frame = node_->getCameraBodyFrame(agent_id_, camera_id);
    std::string camera_optical_frame = node_->getCameraOpticalFrame(agent_id_, camera_id);

    // Initialize camera optical at camera body origin with optical frame rotation
    Matrix4r camera_body_to_camera_optical = Matrix4r::Identity();

    // Apply optical frame rotation: rotate from camera body to optical frame
    // This rotation aligns the camera coordinate system with the optical frame convention
    TransformMsg optical_tf = TransformMsg();
    auto optical_quat = Quaternionr(optical_tf.rotation.w, optical_tf.rotation.x, optical_tf.rotation.y, optical_tf.rotation.z);
    optical_quat *= Quaternionr(0.5, -0.5, 0.5, -0.5);
    camera_body_to_camera_optical.block<3, 3>(0, 0) = MathUtils::quatToMatrix(optical_quat);

    // Broadcast camera body -> camera optical (static)
    node_->broadcastStaticTransform(camera_body_frame, camera_optical_frame, camera_body_to_camera_optical);
    RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", camera_body_frame.c_str(), camera_optical_frame.c_str());
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update frames
// ════════════════════════════════════════════════════════════════════════════

void CameraFrames::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Camera frames: Defaulting update due to invalid status");

        // Set default orientation
        QuaternionMsg orientation_msg;
        orientation_msg.x = 0.0;
        orientation_msg.y = 0.0;
        orientation_msg.z = 0.0;
        orientation_msg.w = 1.0;
        for (const auto& [camera_id, camera_config_ptr] : node_->getSettings()->getMultiCameraSet(agent_id_))
        {
            PointMsg position_msg;
            position_msg.x = camera_config_ptr->position.x();
            position_msg.y = camera_config_ptr->position.y();
            position_msg.z = camera_config_ptr->position.z();

            updateCameraBodyFrame(camera_id, position_msg, orientation_msg);
        }

        return;
    }

    // Iterate through all observation units
    for (int i = 0; i < agent_.observation_setpoints.n_o; ++i)
    {
        // Filter out units that are not cameras
        if (agent_.observation_setpoints.types[i] != static_cast<uint8_t>(ObservationType::Camera))
        {
            continue;
        }

        // Get camera ID
        const ID& camera_id = agent_.observation_setpoints.ids[i];

        // Get camera configuration
        const auto& camera_config_ptr = node_->getSettings()->getMultiCamera(agent_id_, camera_id);

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
        node_->toMsg(quat, orientation_msg);

        // Update frame
        updateCameraBodyFrame(camera_id, position_msg, orientation_msg);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool CameraFrames::checkStatus()
{
    // Check 1: Agent must have a valid observation setpoints
    if (!agent_.has_observation_setpoints)
    {
        RCLCPP_INFO(node_->get_logger(), "Camera frames: Agent %s has no observation setpoints", agent_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// FRAMES UPDATE: Frames update
// ════════════════════════════════════════════════════════════════════════════

void CameraFrames::updateCameraBodyFrame(const ID camera_id, const PointMsg& position, const QuaternionMsg& orientation)
{
    // Get frames
    std::string world_frame = node_->getGlobalFrame();
    std::string body_frame = node_->getAgentBodyFrame(agent_id_);
    std::string camera_body_frame = node_->getCameraBodyFrame(agent_id_, camera_id);

    // Lookup body pose in world using TF
    PoseStampedMsg pose;
    pose.header = node_->createHeader(body_frame);
    pose.pose.position = PointMsg();
    pose.pose.orientation = QuaternionMsg();
    pose.pose.orientation.w = 1.0;
    const PoseStampedMsg wTb = node_->transformPose(pose, world_frame);

    // Extract world to body pose
    const Vector3r wPb(
        wTb.pose.position.x,
        wTb.pose.position.y,
        wTb.pose.position.z);
    const Quaternionr wQb = node_->fromMsg(wTb.pose.orientation);
    const Matrix3r wRb = MathUtils::quatToMatrix(wQb);

    // Camera mounting offset in body frame
    const Vector3r bPc(position.x, position.y, position.z);

    // Get world to camera body position and orientation
    const Vector3r wPc = wPb + (wRb * bPc);
    const Quaternionr wQc = node_->fromMsg(orientation);

    // Build world to camera body transform
    Matrix4r wTc = Matrix4r::Identity();
    wTc(0, 3) = wPc.x();
    wTc(1, 3) = wPc.y();
    wTc(2, 3) = wPc.z();
    wTc.block<3, 3>(0, 0) = MathUtils::quatToMatrix(wQc);

    // Broadcast world -> camera body (dynamic)
    node_->broadcastTransform(world_frame, camera_body_frame, wTc);
}