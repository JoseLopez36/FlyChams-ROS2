#include "flychams_agent/frames/drone_frames.hpp"

using namespace flychams::common;

using namespace flychams::agent;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void DroneFrames::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate
    update_rate_ = node_->getParameterOr<float>("update_rate", 30.0f);

    // Initialize data
    agent_ = Agent();

    // Create PX4 communication
    autopilot_comm_ = std::make_shared<AutopilotCommunication>(agent_id_, node_);

    // Subscribe to topics
    agent_.global_origin_sub = node_->createGlobalOriginSubscriber(
        std::bind(&DroneFrames::globalOriginCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());
    agent_.home_position_sub = autopilot_comm_->subscribeHomePosition(
        std::bind(&DroneFrames::homePositionCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());
    agent_.local_odom_sub = autopilot_comm_->subscribeLocalOdometry(
        std::bind(&DroneFrames::localOdomCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Publish default identity transform world -> local
    std::string world_frame = node_->getGlobalFrame();
    std::string local_frame = node_->getAgentLocalFrame(agent_id_);
    node_->broadcastStaticTransform(world_frame, local_frame, Matrix4r::Identity());
    RCLCPP_INFO(node_->get_logger(), "Drone frames: Published default identity transform %s -> %s", world_frame.c_str(), local_frame.c_str());

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&DroneFrames::update, this));
}

void DroneFrames::onModuleShutdown()
{
    // Destroy subscriber
    agent_.global_origin_sub.reset();
    agent_.home_position_sub.reset();
    agent_.local_odom_sub.reset();
    // Destroy PX4 communication
    autopilot_comm_.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void DroneFrames::globalOriginCallback(const GeoPointStampedMsg::SharedPtr msg)
{
    // Update global origin data
    agent_.global_origin = msg->position;
    agent_.has_global_origin = true;

    // In simulation the global origin equals the home position.
    // Use it as a fallback so frames can be created without waiting for PX4.
    if (!agent_.has_home_position)
    {
        RCLCPP_WARN(node_->get_logger(), "Drone frames: No home position data received. Using global origin as home position for agent %s", agent_id_.c_str());
        agent_.home_position = msg->position;
        agent_.has_home_position = true;

        // Create local frame
        createLocalFrame(agent_.home_position, agent_.global_origin);
    }
}

void DroneFrames::homePositionCallback(const px4_msgs::msg::HomePosition::SharedPtr msg)
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
    agent_.home_position.latitude = msg->lat;
    agent_.home_position.longitude = msg->lon;
    agent_.home_position.altitude = msg->alt;
    agent_.has_home_position = true;

    // Create local frame
    createLocalFrame(agent_.home_position, agent_.global_origin);
}

void DroneFrames::localOdomCallback(const px4_msgs::msg::VehicleOdometry::SharedPtr msg)
{
    // Store odometry data — PX4 VehicleOdometry is NED, convert to ENU
    const Vector3r ned_pos(msg->position[0], msg->position[1], msg->position[2]);
    const Vector3r enu_pos = AutopilotUtils::pointFromNED(ned_pos);
    agent_.local_position.x = enu_pos.x();
    agent_.local_position.y = enu_pos.y();
    agent_.local_position.z = enu_pos.z();

    const Quaternionr ned_q(msg->q[0], msg->q[1], msg->q[2], msg->q[3]);
    const Quaternionr enu_q = AutopilotUtils::px4ToRosOrientation(ned_q);
    agent_.local_orientation.x = enu_q.x();
    agent_.local_orientation.y = enu_q.y();
    agent_.local_orientation.z = enu_q.z();
    agent_.local_orientation.w = enu_q.w();
    agent_.has_local_odom = true;
}

// ════════════════════════════════════════════════════════════════════════════
// FRAMES CREATION: Frames creation
// ════════════════════════════════════════════════════════════════════════════

void DroneFrames::createLocalFrame(const GeoPointMsg& home_geopoint, const GeoPointMsg& origin_geopoint)
{
    // Get frames
    std::string world_frame = node_->getGlobalFrame();
    std::string local_frame = node_->getAgentLocalFrame(agent_id_);

    // Get home position in cartesian coordinates
    PointMsg home_position = FrameUtils::fromGlobal(home_geopoint.latitude, home_geopoint.longitude, home_geopoint.altitude, origin_geopoint);

    // We assume that the home position is at z=0 m (ground level)
    home_position.z = 0.0;

    // Get world to local transformation
    Matrix4r world_to_local = Matrix4r::Identity();
    world_to_local(0, 3) = home_position.y;  // Cross x and y
    world_to_local(1, 3) = -home_position.x; // Cross x and y (invert x)
    world_to_local(2, 3) = home_position.z;

    // Add -90 degrees to yaw
    Quaternionr quat = FrameUtils::eulerToQuat(Vector3r(0.0, 0.0, -M_PIf / 2.0f));
    world_to_local.block<3, 3>(0, 0) = MathUtils::quatToMatrix(quat);

    // Broadcast world -> local (static)
    node_->broadcastStaticTransform(world_frame, local_frame, world_to_local);
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update frames
// ════════════════════════════════════════════════════════════════════════════

void DroneFrames::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Drone frames: Skipping update due to invalid status");
        return;
    }

    // Update body frame with latest odometry data
    updateBodyFrame(agent_.local_position, agent_.local_orientation);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool DroneFrames::checkStatus()
{
    // Check 1: Agent must have a valid global origin
    if (!agent_.has_global_origin)
    {
        RCLCPP_WARN(node_->get_logger(), "Drone frames: Agent %s has no global origin", agent_id_.c_str());
        return false;
    }

    // Check 2: Agent must have a valid home position
    if (!agent_.has_home_position)
    {
        RCLCPP_WARN(node_->get_logger(), "Drone frames: Agent %s has no home position", agent_id_.c_str());
        return false;
    }

    // Check 3: Agent must have a valid local odometry
    if (!agent_.has_local_odom)
    {
        RCLCPP_WARN(node_->get_logger(), "Drone frames: Agent %s has no local odometry", agent_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// FRAMES UPDATE: Frames update
// ════════════════════════════════════════════════════════════════════════════

void DroneFrames::updateBodyFrame(const PointMsg& position, const QuaternionMsg& orientation)
{
    // Get frames
    std::string local_frame = node_->getAgentLocalFrame(agent_id_);
    std::string body_frame = node_->getAgentBodyFrame(agent_id_);

    // Get body transformation
    Matrix4r local_to_body = Matrix4r::Identity();

    // Set position
    local_to_body(0, 3) = position.x;
    local_to_body(1, 3) = position.y;
    local_to_body(2, 3) = position.z;

    // Set orientation
    Quaternionr orientation_quat = node_->fromMsg(orientation);
    local_to_body.block<3, 3>(0, 0) = MathUtils::quatToMatrix(orientation_quat);

    // Broadcast local -> body
    node_->broadcastTransform(local_frame, body_frame, local_to_body);
}