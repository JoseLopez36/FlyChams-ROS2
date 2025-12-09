#include "flychams_control/drone/drone_frames.hpp"

using namespace flychams::core;

namespace flychams::control
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void DroneFrames::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "drone_frames.update_rate", 5.0f);

        // Initialize data
        agent_ = Agent();

        // Create mavros communication
        mavros_comm_ = std::make_shared<MavrosCommunication>(agent_id_, node_, config_tools_, topic_tools_, transform_tools_, module_cb_group_);

        // Create frames
        createLocalFrame();
        createBodyFrame();

        // Iterate through all cameras
        for (const auto& [camera_id, camera_config_ptr] : multi_camera_set)
        {
            createCameraBodyFrame(camera_id, camera_config_ptr);
            createCameraOpticalFrame(camera_id, camera_config_ptr);
        }

        // Subscribe to topics
        agent_.global_origin_sub = topic_tools_->createGlobalOriginSubscriber(
            std::bind(&DroneFrames::globalOriginCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.home_position_sub = mavros_comm_->subscribeHomePosition(
            std::bind(&DroneFrames::homePositionCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.local_odom_sub = mavros_comm_->subscribeLocalOdometry(
            std::bind(&DroneFrames::localOdomCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
    }

    void DroneFrames::onShutdown()
    {
        // Destroy subscriber
        agent_.global_origin_sub.reset();
        agent_.home_position_sub.reset();
        agent_.local_odom_sub.reset();
        // Destroy mavros communication
        mavros_comm_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void DroneFrames::globalOriginCallback(const core::GeoPointStampedMsg::SharedPtr msg)
    {
        // Update global origin data
        agent_.global_origin = *msg;
        agent_.has_global_origin = true;
    }

    void DroneFrames::homePositionCallback(const mavros_msgs::msg::HomePosition::SharedPtr msg)
    {
        // Check if we have a valid global origin
        if (!agent_.has_global_origin)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone frames: No global origin data received. Cannot update local frame for agent %s", agent_id_.c_str());
            return;
        }

        // Update current home position
        agent_.home_position = *msg;
        agent_.has_home_position = true;

        // Create local frame
        updateLocalFrame(agent_.home_position.geopoint, agent_.global_origin.geopoint);
    }

    void DroneFrames::localOdomCallback(const core::OdometryMsg::SharedPtr msg)
    {
        // Check if we have a valid global origin and home position
        if (!agent_.has_global_origin || !agent_.has_home_position)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone frames: No global origin or home position data received. Cannot update body frame for agent %s", agent_id_.c_str());
            return;
        }

        // Update current local odometry
        agent_.local_odom = *msg;
        agent_.has_local_odom = true;

        // Update body frame
        const auto& position = agent_.local_odom.pose.pose.position;
        const auto& orientation = agent_.local_odom.pose.pose.orientation;
        updateBodyFrame(position, orientation);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // FRAMES CREATION: Frames creation
    // ════════════════════════════════════════════════════════════════════════════

    void DroneFrames::createLocalFrame()
    {
        // Get frames
        std::string world_frame = transform_tools_->getGlobalFrame();
        std::string local_frame = transform_tools_->getAgentLocalFrame(agent_id_);

        // Initialize local frame at world origin
        Matrix4r world_to_local = Matrix4r::Identity();

        // Broadcast world -> local
        transform_tools_->broadcastTransform(world_frame, local_frame, world_to_local);
        RCLCPP_INFO(node_->get_logger(), "Published transform: %s -> %s", agent_.world_frame.c_str(), agent_.local_frame.c_str());
    }

    void DroneFrames::createBodyFrame()
    {
        // Get frames
        std::string local_frame = transform_tools_->getAgentLocalFrame(agent_id_);
        std::string body_frame = transform_tools_->getAgentBodyFrame(agent_id_);

        // Initialize drone body at local origin
        Matrix4r local_to_body = Matrix4r::Identity();

        // Broadcast local -> body
        transform_tools_->broadcastTransform(local_frame, body_frame, local_to_body);
        RCLCPP_INFO(node_->get_logger(), "Published transform: %s -> %s", agent_.local_frame.c_str(), agent_.body_frame.c_str());
    }

    void DroneFrames::createCameraBodyFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr)
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
        RCLCPP_INFO(node_->get_logger(), "Published transform: %s -> %s", agent_.body_frame.c_str(), agent_.camera_body_frame.c_str());
    }

    void DroneFrames::createCameraOpticalFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr)
    {
        // Get frames
        std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);
        std::string camera_optical_frame = transform_tools_->getCameraOpticalFrame(agent_id_, camera_id);

        // Initialize camera optical at camera body origin
        Matrix4r camera_body_to_camera_optical = Matrix4r::Identity();

        // Broadcast camera body -> camera optical (static)
        transform_tools_->broadcastStaticTransform(camera_body_frame, camera_optical_frame, camera_body_to_camera_optical);
        RCLCPP_INFO(node_->get_logger(), "Published static transform: %s -> %s", agent_.camera_body_frame.c_str(), agent_.camera_optical_frame.c_str());
    }

    // ════════════════════════════════════════════════════════════════════════════
    // FRAMES UPDATE: Frames update
    // ════════════════════════════════════════════════════════════════════════════

    void DroneFrames::updateLocalFrame(const core::GeoPointMsg& home_geopoint, const core::GeoPointMsg& origin_geopoint)
    {
        // Get frames
        std::string world_frame = transform_tools_->getGlobalFrame();
        std::string local_frame = transform_tools_->getAgentLocalFrame(agent_id_);

        // Get home position in cartesian coordinates
        PointMsg home_position = GeoUtils::toLocal(home_geopoint.latitude, home_geopoint.longitude, home_geopoint.altitude, origin_geopoint);

        // We assume that the home position is at z=0 m (ground level)
        home_position.z = 0.0;

        // Get world to local transformation
        // We assume that the local frame is aligned with the world frame (ENU)
        Matrix4r world_to_local = Matrix4r::Identity();
        world_to_local.block(0, 3) = home_position.point.x;
        world_to_local.block(1, 3) = home_position.point.y;
        world_to_local.block(2, 3) = home_position.point.z;

        // Broadcast world -> local
        transform_tools_->broadcastTransform(world_frame, local_frame, world_to_local);
        RCLCPP_INFO(node_->get_logger(), "Drone frames: Published static transform: %s -> %s", agent_.world_frame.c_str(), agent_.local_frame.c_str());
    }

    void DroneFrames::updateBodyFrame(const core::PointMsg& position, const core::QuaternionMsg& orientation)
    {
        // Get frames
        std::string local_frame = transform_tools_->getAgentLocalFrame(agent_id_);
        std::string body_frame = transform_tools_->getAgentBodyFrame(agent_id_);

        // Get body transformation
        Matrix4r local_to_body = Matrix4r::Identity();

        // Set position
        local_to_body.block(0, 3) = position.point.x;
        local_to_body.block(1, 3) = position.point.y;
        local_to_body.block(2, 3) = position.point.z;

        // Set orientation
        Quaternionr orientation_quat = MathUtils::fromMsg(orientation);
        local_to_body.block<3, 3>(0, 0) = MathUtils::quaternionToRotationMatrix(orientation_quat);

        // Broadcast local -> body
        transform_tools_->broadcastTransform(local_frame, body_frame, local_to_body);
        RCLCPP_INFO(node_->get_logger(), "Drone frames: Published transform: %s -> %s", agent_.local_frame.c_str(), agent_.body_frame.c_str());
    }

    void DroneFrames::updateCameraBodyFrame(const core::ID camera_id, const core::PointMsg& position, const core::QuaternionMsg& orientation)
    {
        // Get frames
        std::string body_frame = transform_tools_->getAgentBodyFrame(agent_id_);
        std::string camera_body_frame = transform_tools_->getCameraBodyFrame(agent_id_, camera_id);

        // Get body to camera body transformation
        Matrix4r body_to_camera_body = Matrix4r::Identity();

        // Set position
        body_to_camera_body.block(0, 3) = position.point.x;
        body_to_camera_body.block(1, 3) = position.point.y;
        body_to_camera_body.block(2, 3) = position.point.z;

        // Set orientation
        Quaternionr orientation_quat = MathUtils::fromMsg(orientation);
        body_to_camera_body.block<3, 3>(0, 0) = MathUtils::quaternionToRotationMatrix(orientation_quat);

        // Broadcast agent body -> camera body
        transform_tools_->broadcastTransform(body_frame, camera_body_frame, body_to_camera_body);
        RCLCPP_INFO(node_->get_logger(), "Drone frames: Published transform: %s -> %s", body_frame.c_str(), camera_body_frame.c_str());
    }

} // namespace flychams::control
