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
        agent_.global_origin = msg->position;
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

        // Check if we have a valid home position
        if (agent_.has_home_position)
        {
            // We already have a home position, we don't need to update it
            return;
        }

        // Update current home position
        agent_.home_position = msg->geo;
        agent_.has_home_position = true;

        // Create local frame
        createLocalFrame(agent_.home_position, agent_.global_origin);
    }

    void DroneFrames::localOdomCallback(const core::OdometryMsg::SharedPtr msg)
    {
        // Check if we have a valid global origin and home position
        if (!agent_.has_global_origin || !agent_.has_home_position)
        {
            RCLCPP_WARN(node_->get_logger(), "Drone frames: No global origin or home position data received. Cannot update body frame for agent %s", agent_id_.c_str());
            return;
        }
        // Update body frame
        const auto& position = msg->pose.pose.position;
        const auto& orientation = msg->pose.pose.orientation;
        updateBodyFrame(position, orientation);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // FRAMES CREATION: Frames creation
    // ════════════════════════════════════════════════════════════════════════════

    void DroneFrames::createLocalFrame(const core::GeoPointMsg& home_geopoint, const core::GeoPointMsg& origin_geopoint)
    {
        // Get frames
        std::string world_frame = transform_tools_->getGlobalFrame();
        std::string local_frame = transform_tools_->getAgentLocalFrame(agent_id_);

        // Get home position in cartesian coordinates
        PointMsg home_position = TfUtils::fromGlobal(home_geopoint.latitude, home_geopoint.longitude, home_geopoint.altitude, origin_geopoint);

        // We assume that the home position is at z=0 m (ground level)
        home_position.z = 0.0;

        // Get world to local transformation
        Matrix4r world_to_local = Matrix4r::Identity();
        world_to_local(0, 3) = home_position.y;  // Cross x and y
        world_to_local(1, 3) = -home_position.x; // Cross x and y (invert x)
        world_to_local(2, 3) = home_position.z;

        // Add -90 degrees to yaw
        Quaternionr quat = TfUtils::eulerToQuat(Vector3r(0.0, 0.0, -M_PIf / 2.0f));
        world_to_local.block<3, 3>(0, 0) = TfUtils::quatToMatrix(quat);

        // Broadcast world -> local (static)
        transform_tools_->broadcastStaticTransform(world_frame, local_frame, world_to_local);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // FRAMES UPDATE: Frames update
    // ════════════════════════════════════════════════════════════════════════════

    void DroneFrames::updateBodyFrame(const core::PointMsg& position, const core::QuaternionMsg& orientation)
    {
        // Get frames
        std::string local_frame = transform_tools_->getAgentLocalFrame(agent_id_);
        std::string body_frame = transform_tools_->getAgentBodyFrame(agent_id_);

        // Get body transformation
        Matrix4r local_to_body = Matrix4r::Identity();

        // Set position
        local_to_body(0, 3) = position.x;
        local_to_body(1, 3) = position.y;
        local_to_body(2, 3) = position.z;

        // Set orientation
        Quaternionr orientation_quat = RosUtils::fromMsg(orientation);
        local_to_body.block<3, 3>(0, 0) = TfUtils::quatToMatrix(orientation_quat);

        // Broadcast local -> body
        transform_tools_->broadcastTransform(local_frame, body_frame, local_to_body);
    }

} // namespace flychams::control
