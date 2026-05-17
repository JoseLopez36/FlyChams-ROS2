#include "flychams_simulation/bridge/agent_bridge.hpp"

using namespace flychams::common;

using namespace flychams::simulation;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void AgentBridge::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate
    update_rate_ = node_->getParameterOr<float>("bridge_update_rate", 20.0f);

    // Initialize data
    agent_ = Agent();

    // Create subscriber for agent observation setpoints
    agent_.observation_setpoints_sub = node_->createObservationSetpointsSubscriber(agent_id_,
        std::bind(&AgentBridge::observationSetpointsCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Create publishers for AirSim commands
    // Camera FOV command topic: /airsim/vehicles/cmd/camera_fov
    agent_.camera_fov_cmd_pub = node_->create_publisher<CameraFovCmdMsg>("/airsim/vehicles/cmd/camera_fov", 10);
    // Gimbal angle command topic: /airsim/vehicles/cmd/gimbal_angle
    agent_.gimbal_angle_cmd_pub = node_->create_publisher<GimbalAngleCmdMsg>("/airsim/vehicles/cmd/gimbal_angle", 10);

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentBridge::update, this));
}

void AgentBridge::onModuleShutdown()
{
    // Destroy agent data
    agent_.observation_setpoints_sub.reset();
    agent_.camera_fov_cmd_pub.reset();
    agent_.gimbal_angle_cmd_pub.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void AgentBridge::observationSetpointsCallback(const ObservationSetpointsMsg::SharedPtr msg)
{
    // Update observation setpoints
    agent_.observation_setpoints = *msg;
    agent_.has_observation_setpoints = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update bridge
// ════════════════════════════════════════════════════════════════════════════

void AgentBridge::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Agent bridge: Skipping update due to invalid status");
        return;
    }

    // Publish gimbal/camera commands
    publishGimbalAngleCmd();
    publishCameraFovCmd();
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool AgentBridge::checkStatus()
{
    // Check 1: Agent must have valid observation setpoints
    if (!agent_.has_observation_setpoints)
    {
        RCLCPP_WARN_THROTTLE(node_->get_logger(), *node_->get_clock(), 5000,
            "Agent simulation bridge: Agent %s has no observation setpoints", agent_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLISHING
// ════════════════════════════════════════════════════════════════════════════

void AgentBridge::publishGimbalAngleCmd()
{
    // Create gimbal angle command message
    GimbalAngleCmdMsg msg;
    msg.header = node_->createHeader(node_->getGlobalFrame());

    // Extract gimbal angle data from observation setpoints
    for (int i = 0; i < agent_.observation_setpoints.n_o; ++i)
    {
        // Filter out units that are not cameras
        if (agent_.observation_setpoints.types[i] != static_cast<uint8_t>(ObservationType::Camera))
        {
            continue;
        }

        // Set vehicle name
        msg.vehicle_name = agent_id_;

        // Add camera name
        msg.camera_names.push_back(agent_.observation_setpoints.ids[i]);

        // Convert rotation (RPY) to quaternion using Eigen
        const auto& rotation = agent_.observation_setpoints.rotations[i];
        Vector3r rpy_vec(rotation.x, rotation.y, rotation.z);

        // Eigen Euler to Quaternion conversion (Z-Y-X order: yaw-pitch-roll)
        Quaternionr quat =
            Eigen::AngleAxisf(rpy_vec.z(), Vector3r::UnitZ()) *
            Eigen::AngleAxisf(rpy_vec.y(), Vector3r::UnitY()) *
            Eigen::AngleAxisf(rpy_vec.x(), Vector3r::UnitX());

        QuaternionMsg quat_msg;
        node_->toMsg(quat, quat_msg);
        msg.orientations.push_back(quat_msg);
    }

    // Publish message
    agent_.gimbal_angle_cmd_pub->publish(msg);
}

void AgentBridge::publishCameraFovCmd()
{
    // Create camera FOV command message
    CameraFovCmdMsg msg;
    msg.header = node_->createHeader(node_->getGlobalFrame());

    // Extract camera FOV data from observation setpoints
    for (int i = 0; i < agent_.observation_setpoints.n_o; ++i)
    {
        // Filter out units that are not cameras
        if (agent_.observation_setpoints.types[i] != static_cast<uint8_t>(ObservationType::Camera))
        {
            continue;
        }

        // Get camera ID
        const ID& camera_id = agent_.observation_setpoints.ids[i];

        // Set vehicle name
        msg.vehicle_name = agent_id_;

        // Add camera name
        msg.camera_names.push_back(camera_id);

        // Get camera configuration for sensor size
        const auto& camera_config = node_->getSettings()->getCamera(agent_id_, camera_id);
        const float& sensor_width = camera_config.sensor_size(0);

        // Calculate camera FoV
        const float& focal = agent_.observation_setpoints.zoom_factors[i];
        float fov = VisionUtils::computeFov(focal, sensor_width);
        msg.fovs.push_back(fov);
    }

    // Publish message
    agent_.camera_fov_cmd_pub->publish(msg);
}