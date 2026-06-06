#include "flychams_common/settings/settings_tools.hpp"

using namespace flychams::common;

SettingsTools::SettingsTools(NodePtr node)
    : node_(node)
{
    // Initialize config pointer
    config_ptr_ = std::make_shared<MissionConfig>();

    // Set prefixes for parameters (matching config_parser.cpp)
    std::string mission_prefix = "mission.";
    std::string environment_prefix = "environment.";
    std::string targets_prefix = "targets.";
    std::string agents_prefix = "agents.";

    // Parse mission parameters from ROS2 parameters server
    MissionSettingsParser::parseMissionParameters(node_, mission_prefix, config_ptr_);
    MissionSettingsParser::parseEnvironmentParameters(node_, environment_prefix, config_ptr_);
    MissionSettingsParser::parseTargetParameters(node_, targets_prefix, config_ptr_);
    MissionSettingsParser::parseAgentParameters(node_, agents_prefix, config_ptr_);

    // Parse system, topics and frames parameters from ROS2 parameters server
    MissionSettingsParser::parseSystemParameters(node_, config_ptr_);
    MissionSettingsParser::parseTopicParameters(node_, config_ptr_);
    MissionSettingsParser::parseFrameParameters(node_, config_ptr_);

    // Print settings
    printSettings();
}

const TrackingParameters SettingsTools::getTrackingParameters(const std::string& agent_id) const
{
    TrackingParameters params;

    // Extract tracking config
    const auto& tracking = getTracking(agent_id);

    // Get number of units
    params.n_c = static_cast<int>(tracking.multi_camera_set.size()) - 1;
    params.n_w = static_cast<int>(tracking.multi_window_set.size());
    params.n_t = params.n_c + params.n_w;
    params.n_o = params.n_t + 1;

    // Get tracking mode based on number of multi-cameras and multi-windows
    if (params.n_t <= 0)
    {
        params.mode = TrackingMode::None;
        RCLCPP_INFO(node_->get_logger(), "Tracking mode for agent %s: None", agent_id.c_str());
    }
    else
    {
        if (params.n_w == 0)
        {
            params.mode = TrackingMode::MultiCamera;
            RCLCPP_INFO(node_->get_logger(), "Tracking mode for agent %s: MultiCamera (n_c = %d)", agent_id.c_str(), params.n_c);
        }
        else if (params.n_w > 0 && params.n_c == 0)
        {
            params.mode = TrackingMode::MultiWindow;
            RCLCPP_INFO(node_->get_logger(), "Tracking mode for agent %s: MultiWindow (n_w = %d)", agent_id.c_str(), params.n_w);
        }
        else if (params.n_w > 0 && params.n_c > 0)
        {
            params.mode = TrackingMode::MultiHybrid;
            RCLCPP_INFO(node_->get_logger(), "Tracking mode for agent %s: MultiHybrid (n_c = %d, n_w = %d)", agent_id.c_str(), params.n_c, params.n_w);
        }
        else
        {
            throw std::runtime_error("Invalid number of multi-cameras and multi-windows");
        }
    }

    // Set parameters for each unit (first the multi-camera set, then the multi-window set)
    params.observation_units_params.resize(params.n_o);
    int i = 1;
    for (const auto& [multi_camera_id, multi_camera] : tracking.multi_camera_set)
    {
        if (multi_camera->role == ObservationRole::Central)
        {
            params.observation_units_params[0] = getObservationUnitParameters(tracking, multi_camera);
        }
        else if (multi_camera->role == ObservationRole::Tracking)
        {
            params.observation_units_params[i] = getObservationUnitParameters(tracking, multi_camera);
            i++;
        }
    }
    for (const auto& [multi_window_id, multi_window] : tracking.multi_window_set)
    {
        params.observation_units_params[i] = getObservationUnitParameters(tracking, params.observation_units_params[0], multi_window);
        i++;
    }

    return params;
}

const ObservationUnitParameters SettingsTools::getObservationUnitParameters(const TrackingConfig& tracking, const MultiCameraConfigPtr& multi_camera) const
{
    // Method to get the parameters for a multi-camera unit
    ObservationUnitParameters params;

    // Unit ID
    params.id = multi_camera->id;

    // Unit type
    params.type = ObservationType::Camera;

    // Unit role
    params.role = multi_camera->role;

    // Camera focal length limits (m)
    params.upsilon_min = multi_camera->min_focal;
    params.upsilon_max = multi_camera->max_focal;
    params.upsilon_ref = multi_camera->ref_focal;

    // Get camera parameters
    // Camera resolution (pix)
    params.camera_params.width = multi_camera->camera.resolution(0);
    params.camera_params.height = multi_camera->camera.resolution(1);
    // Camera sensor dimensions (m)
    params.camera_params.sensor_width = multi_camera->camera.sensor_size(0);
    params.camera_params.sensor_height = multi_camera->camera.sensor_size(1);
    // Regularized pixel size (m/pix)
    params.rho_x = params.camera_params.sensor_width / static_cast<float>(params.camera_params.width);       // [m/pix]
    params.rho_y = params.camera_params.sensor_height / static_cast<float>(params.camera_params.height);     // [m/pix]
    params.rho = std::sqrt(params.rho_x * params.rho_y);                                                     // [m/pix]
    // Camera reference intrinsic matrix K
    params.camera_params.K = Matrix3r::Identity();
    params.camera_params.K(0, 0) = params.upsilon_ref / params.rho_x;
    params.camera_params.K(1, 1) = params.upsilon_ref / params.rho_y;
    params.camera_params.K(0, 2) = params.camera_params.width / 2.0f;
    params.camera_params.K(1, 2) = params.camera_params.height / 2.0f;

    // Calculate ROI parameters
    const auto& min_apparent_size = tracking.min_target_size;
    const auto& max_apparent_size = tracking.max_target_size;
    const auto& ref_apparent_size = tracking.ref_target_size;
    float sensor_half_size = static_cast<float>(std::min(params.camera_params.width, params.camera_params.height)) / 2.0f;

    // Minimum admissible apparent size of the object in the image (in pixels)
    float s_min_pix = sensor_half_size * min_apparent_size;
    params.s_min_pix = s_min_pix; // [pix]

    // Maximum admissible apparent size of the object in the image (in pixels)
    float s_max_pix = sensor_half_size * max_apparent_size;
    params.s_max_pix = s_max_pix; // [pix]

    // Reference apparent size of the object in the image (in pixels)
    float s_ref_pix = sensor_half_size * ref_apparent_size; // [pix]
    params.s_ref_pix = s_ref_pix; // [pix]

    // Conversion to metric distances on the sensor surface
    params.s_max = s_max_pix * params.rho; // [m]
    params.s_min = s_min_pix * params.rho; // [m]
    params.s_ref = s_ref_pix * params.rho; // [m]  

    // Print unit parameters for debugging
    RCLCPP_DEBUG(node_->get_logger(), "------ Observation unit parameters -------");
    RCLCPP_DEBUG(node_->get_logger(), "  Unit ID: %s", params.id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Unit type: %s", observationTypeToString(params.type).c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Unit role: %s", observationRoleToString(params.role).c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Upsilon limits: min=%.3f, max=%.3f, ref=%.3f [m]", params.upsilon_min, params.upsilon_max, params.upsilon_ref);
    RCLCPP_DEBUG(node_->get_logger(), "  Regularized pixel size: %.6f [m/pix]", params.rho);
    RCLCPP_DEBUG(node_->get_logger(), "  Target size limits: min=%.2f [pix], max=%.2f [pix], ref=%.2f [pix]", params.s_min_pix, params.s_max_pix, params.s_ref_pix);
    RCLCPP_DEBUG(node_->get_logger(), "  Camera resolution: %d x %d [pix]", params.camera_params.width, params.camera_params.height);
    RCLCPP_DEBUG(node_->get_logger(), "  Sensor dimensions: %.6f x %.6f [m]", params.camera_params.sensor_width, params.camera_params.sensor_height);
    RCLCPP_DEBUG(node_->get_logger(), "  Intrinsic matrix K: fx=%f fy=%f cx=%f cy=%f", params.camera_params.K(0, 0), params.camera_params.K(1, 1), params.camera_params.K(0, 2), params.camera_params.K(1, 2));
    RCLCPP_DEBUG(node_->get_logger(), "----------------------------------------");

    return params;
}

const ObservationUnitParameters SettingsTools::getObservationUnitParameters(const TrackingConfig& tracking, const ObservationUnitParameters& central_camera_params, const MultiWindowConfigPtr& multi_window) const
{
    // Method to get the parameters for a multi-window unit
    ObservationUnitParameters params;

    // Unit ID
    params.id = multi_window->id;

    // Unit type
    params.type = ObservationType::Window;

    // Unit role (tracking always for multi-window units)
    params.role = ObservationRole::Tracking;

    // Get central camera parameters
    params.camera_params = central_camera_params.camera_params;

    // Get window parameters
    // Resolution factor limits (config)
    params.window_params.lambda_min = multi_window->min_lambda;
    params.window_params.lambda_max = multi_window->max_lambda;
    params.window_params.lambda_ref = multi_window->ref_lambda;
    // Central camera focal length (m)
    params.window_params.f_ref = central_camera_params.upsilon_ref;
    // Full resolution (pix)
    params.window_params.full_width = central_camera_params.camera_params.width;
    params.window_params.full_height = central_camera_params.camera_params.height;
    // Tracking resolution (pix)
    params.window_params.tracking_width = multi_window->resolution(0);
    params.window_params.tracking_height = multi_window->resolution(1);
    // Regularized pixel size (m/pix)
    params.rho_x = central_camera_params.rho_x;
    params.rho_y = central_camera_params.rho_y;
    params.rho = central_camera_params.rho;

    // Zoom factor (upsilon) limits: upsilon = lambda * xi
    const Vector2r c(params.camera_params.K(0, 2), params.camera_params.K(1, 2));
    std::tie(params.upsilon_min, params.upsilon_max, params.upsilon_ref) = ZoomUtils::computeWindowUpsilonBounds(
        params.window_params.full_width, params.window_params.full_height, c,
        params.rho_x, params.rho_y,
        params.window_params.lambda_min, params.window_params.lambda_max, params.window_params.lambda_ref,
        params.window_params.f_ref);

    // Calculate ROI parameters
    const auto& min_apparent_size = tracking.min_target_size;
    const auto& max_apparent_size = tracking.max_target_size;
    const auto& ref_apparent_size = tracking.ref_target_size;
    float sensor_half_size = static_cast<float>(std::min(params.window_params.tracking_width, params.window_params.tracking_height)) / 2.0f;

    // Minimum admissible apparent size of the object in the image (in pixels)
    float s_min_pix = sensor_half_size * min_apparent_size;
    params.s_min_pix = s_min_pix; // [pix]

    // Maximum admissible apparent size of the object in the image (in pixels)
    float s_max_pix = sensor_half_size * max_apparent_size;
    params.s_max_pix = s_max_pix; // [pix]

    // Reference apparent size of the object in the image (in pixels)
    float s_ref_pix = sensor_half_size * ref_apparent_size; // [pix]
    params.s_ref_pix = s_ref_pix; // [pix]

    // Conversion to metric distances on the sensor surface
    params.s_max = s_max_pix * params.rho; // [m]
    params.s_min = s_min_pix * params.rho; // [m]
    params.s_ref = s_ref_pix * params.rho; // [m]  

    // Print unit parameters for debugging
    RCLCPP_DEBUG(node_->get_logger(), "------ Observation unit parameters -------");
    RCLCPP_DEBUG(node_->get_logger(), "  Unit ID: %s", params.id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Unit type: %s", observationTypeToString(params.type).c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Unit role: %s", observationRoleToString(params.role).c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Resolution factor limits: min=%.3f, max=%.3f, ref=%.3f", params.window_params.lambda_min, params.window_params.lambda_max, params.window_params.lambda_ref);
    RCLCPP_DEBUG(node_->get_logger(), "  Zoom factor (upsilon) limits: min=%.3f, max=%.3f, ref=%.3f", params.upsilon_min, params.upsilon_max, params.upsilon_ref);
    RCLCPP_DEBUG(node_->get_logger(), "  Regularized pixel size: %.6f [m/pix]", params.rho);
    RCLCPP_DEBUG(node_->get_logger(), "  Target size limits: min=%.2f [pix], max=%.2f [pix], ref=%.2f [pix]", params.s_min_pix, params.s_max_pix, params.s_ref_pix);
    RCLCPP_DEBUG(node_->get_logger(), "  Window full resolution: %d x %d [pix]", params.window_params.full_width, params.window_params.full_height);
    RCLCPP_DEBUG(node_->get_logger(), "  Window tracking resolution: %d x %d [pix]", params.window_params.tracking_width, params.window_params.tracking_height);
    RCLCPP_DEBUG(node_->get_logger(), "----------------------------------------");

    return params;
}

// Helper functions for enum-to-string conversions
std::string autopilotToString(const Autopilot& autopilot)
{
    switch (autopilot)
    {
    case Autopilot::SimpleFlight: return "SimpleFlight";
    case Autopilot::PX4: return "PX4";
    default: return "None";
    }
}

std::string targetTypeToString(const TargetType& type)
{
    switch (type)
    {
    case TargetType::Human: return "Human";
    default: return "None";
    }
}

std::string priorityToString(const Priority& priority)
{
    switch (priority)
    {
    case Priority::Low: return "Low";
    case Priority::Medium: return "Medium";
    case Priority::High: return "High";
    default: return "None";
    }
}

std::string droneTypeToString(const DroneType& type)
{
    switch (type)
    {
    case DroneType::Quadcopter: return "Quadcopter";
    case DroneType::Hexacopter: return "Hexacopter";
    default: return "None";
    }
}

std::string cameraTypeToString(const CameraType& type)
{
    switch (type)
    {
    case CameraType::RGB: return "RGB";
    case CameraType::Infrared: return "Infrared";
    case CameraType::Depth: return "Depth";
    default: return "None";
    }
}

std::string simulationFrameworkToString(const SimulationFramework& framework)
{
    switch (framework)
    {
    case SimulationFramework::AirSim: return "AirSim";
    case SimulationFramework::Gazebo: return "Gazebo";
    case SimulationFramework::IsaacSim: return "IsaacSim";
    default: return "None";
    }
}

// Utility methods
void SettingsTools::printSettings() const
{
    RCLCPP_DEBUG(node_->get_logger(), "═══════════════════════════════════════════════════════════════");
    RCLCPP_DEBUG(node_->get_logger(), "                    MISSION SETTINGS");
    RCLCPP_DEBUG(node_->get_logger(), "═══════════════════════════════════════════════════════════════");

    // Mission Parameters
    RCLCPP_DEBUG(node_->get_logger(), "\n[MISSION]");
    RCLCPP_DEBUG(node_->get_logger(), "  ID: %s", config_ptr_->id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Name: %s", config_ptr_->name.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Environment ID: %s", config_ptr_->environment_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Target Group ID: %s", config_ptr_->target_group_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Agent Team ID: %s", config_ptr_->agent_team_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Horizontal Constraint: [%.3f, %.3f]",
        config_ptr_->horizontal_constraint(0), config_ptr_->horizontal_constraint(1));
    RCLCPP_DEBUG(node_->get_logger(), "  Vertical Constraint: [%.3f, %.3f]",
        config_ptr_->vertical_constraint(0), config_ptr_->vertical_constraint(1));
    RCLCPP_DEBUG(node_->get_logger(), "  Autopilot: %s", autopilotToString(config_ptr_->autopilot).c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Start Date: %d/%02d/%02d",
        config_ptr_->start_date.year, config_ptr_->start_date.month, config_ptr_->start_date.day);
    RCLCPP_DEBUG(node_->get_logger(), "  Start Hour: %02d:%02d:%02d",
        config_ptr_->start_hour.hours, config_ptr_->start_hour.minutes, config_ptr_->start_hour.seconds);

// Environment Parameters
    RCLCPP_DEBUG(node_->get_logger(), "\n[ENVIRONMENT]");
    RCLCPP_DEBUG(node_->get_logger(), "  ID: %s", config_ptr_->environment.id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Name: %s", config_ptr_->environment.name.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Geopoint: lat=%.6f, lon=%.6f, alt=%.3f",
        config_ptr_->environment.geopoint.latitude,
        config_ptr_->environment.geopoint.longitude,
        config_ptr_->environment.geopoint.altitude);
    RCLCPP_DEBUG(node_->get_logger(), "  Wind Velocity: [%.3f, %.3f, %.3f]",
        config_ptr_->environment.wind_vel.x(),
        config_ptr_->environment.wind_vel.y(),
        config_ptr_->environment.wind_vel.z());

// Target Parameters
    RCLCPP_DEBUG(node_->get_logger(), "\n[TARGETS]");
    RCLCPP_DEBUG(node_->get_logger(), "  Total Targets: %zu", config_ptr_->target_group.size());
    for (const auto& [target_id, target] : config_ptr_->target_group)
    {
        RCLCPP_DEBUG(node_->get_logger(), "  [%s]", target_id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Name: %s", target->name.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Target Group ID: %s", target->target_group_id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Target Index: %d", target->target_index);
        RCLCPP_DEBUG(node_->get_logger(), "    Type: %s", targetTypeToString(target->type).c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Count: %d", target->count);
        RCLCPP_DEBUG(node_->get_logger(), "    Priority: %s", priorityToString(target->priority).c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Trajectory Folder: %s", target->trajectory_folder.c_str());
    }

    // Agent Parameters
    RCLCPP_DEBUG(node_->get_logger(), "\n[AGENTS]");
    RCLCPP_DEBUG(node_->get_logger(), "  Total Agents: %zu", config_ptr_->agent_team.size());
    for (const auto& [agent_id, agent] : config_ptr_->agent_team)
    {
        RCLCPP_DEBUG(node_->get_logger(), "  [%s]", agent_id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Name: %s", agent->name.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Agent Team ID: %s", agent->agent_team_id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Tracking ID: %s", agent->tracking_id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Drone ID: %s", agent->drone_id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "    Position: [%.3f, %.3f, %.3f]",
            agent->position.x(), agent->position.y(), agent->position.z());
        RCLCPP_DEBUG(node_->get_logger(), "    Orientation: [roll=%.3f, pitch=%.3f, yaw=%.3f]",
            agent->orientation.x(), agent->orientation.y(), agent->orientation.z());
        RCLCPP_DEBUG(node_->get_logger(), "    Safety Radius: %.3f", agent->safety_radius);
        RCLCPP_DEBUG(node_->get_logger(), "    Max Altitude: %.3f", agent->max_altitude);
        RCLCPP_DEBUG(node_->get_logger(), "    Battery Capacity: %.3f", agent->battery_capacity);

        // Tracking Parameters
        RCLCPP_DEBUG(node_->get_logger(), "    [TRACKING]");
        RCLCPP_DEBUG(node_->get_logger(), "      ID: %s", agent->tracking.id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "      Name: %s", agent->tracking.name.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "      Observation Set ID: %s", agent->tracking.observation_set_id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "      Target Size: min=%.3f, max=%.3f, ref=%.3f",
            agent->tracking.min_target_size,
            agent->tracking.max_target_size,
            agent->tracking.ref_target_size);
        RCLCPP_DEBUG(node_->get_logger(), "      Multi-Cameras: %zu", agent->tracking.multi_camera_set.size());
        RCLCPP_DEBUG(node_->get_logger(), "      Multi-Windows: %zu", agent->tracking.multi_window_set.size());

        // Multi-Camera Parameters
        for (const auto& [multi_camera_id, multi_camera] : agent->tracking.multi_camera_set)
        {
            RCLCPP_DEBUG(node_->get_logger(), "      [MULTI-CAMERA: %s]", multi_camera_id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Name: %s", multi_camera->name.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Observation Set ID: %s", multi_camera->observation_set_id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Camera ID: %s", multi_camera->camera_id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Gimbal ID: %s", multi_camera->gimbal_id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Role: %s", observationRoleToString(multi_camera->role).c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Position: [%.3f, %.3f, %.3f]",
                multi_camera->position.x(), multi_camera->position.y(), multi_camera->position.z());
            RCLCPP_DEBUG(node_->get_logger(), "        Orientation: [roll=%.3f, pitch=%.3f, yaw=%.3f]",
                multi_camera->orientation.x(), multi_camera->orientation.y(), multi_camera->orientation.z());
            RCLCPP_DEBUG(node_->get_logger(), "        Focal Length: min=%.6f, max=%.6f, ref=%.6f",
                multi_camera->min_focal, multi_camera->max_focal, multi_camera->ref_focal);
            RCLCPP_DEBUG(node_->get_logger(), "        Source Stream URL: %s", multi_camera->source_stream_url.c_str());

            // Camera Parameters
            RCLCPP_DEBUG(node_->get_logger(), "        [CAMERA]");
            RCLCPP_DEBUG(node_->get_logger(), "          ID: %s", multi_camera->camera.id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "          Name: %s", multi_camera->camera.name.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "          Type: %s", cameraTypeToString(multi_camera->camera.type).c_str());
            RCLCPP_DEBUG(node_->get_logger(), "          Resolution: %d x %d",
                multi_camera->camera.resolution(0), multi_camera->camera.resolution(1));
            RCLCPP_DEBUG(node_->get_logger(), "          Sensor Size: %.6f x %.6f",
                multi_camera->camera.sensor_size(0), multi_camera->camera.sensor_size(1));
            RCLCPP_DEBUG(node_->get_logger(), "          Distortion: K1=%.6f, K2=%.6f, K3=%.6f, P1=%.6f, P2=%.6f",
                multi_camera->camera.distortion.K1,
                multi_camera->camera.distortion.K2,
                multi_camera->camera.distortion.K3,
                multi_camera->camera.distortion.P1,
                multi_camera->camera.distortion.P2);
            RCLCPP_DEBUG(node_->get_logger(), "          Sensor Noise Enabled: %s",
                multi_camera->camera.enable_sensor_noise ? "true" : "false");
            if (multi_camera->camera.enable_sensor_noise)
            {
                RCLCPP_DEBUG(node_->get_logger(), "          Sensor Noise: rand_contrib=%.6f, rand_size=%.3f, rand_speed=%.3f",
                    multi_camera->camera.sensor_noise.rand_contrib,
                    multi_camera->camera.sensor_noise.rand_size,
                    multi_camera->camera.sensor_noise.rand_speed);
            }
            RCLCPP_DEBUG(node_->get_logger(), "          Weight: %.3f", multi_camera->camera.weight);
            RCLCPP_DEBUG(node_->get_logger(), "          Power: idle=%.3f, active=%.3f",
                multi_camera->camera.idle_power, multi_camera->camera.active_power);

            // Gimbal Parameters
            RCLCPP_DEBUG(node_->get_logger(), "        [GIMBAL]");
            RCLCPP_DEBUG(node_->get_logger(), "          ID: %s", multi_camera->gimbal.id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "          Name: %s", multi_camera->gimbal.name.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "          Roll: enabled=%s, [%.3f, %.3f], max_speed=%.3f",
                multi_camera->gimbal.enable_roll ? "true" : "false",
                multi_camera->gimbal.roll.min_angle,
                multi_camera->gimbal.roll.max_angle,
                multi_camera->gimbal.roll.max_speed);
            RCLCPP_DEBUG(node_->get_logger(), "          Pitch: enabled=%s, [%.3f, %.3f], max_speed=%.3f",
                multi_camera->gimbal.enable_pitch ? "true" : "false",
                multi_camera->gimbal.pitch.min_angle,
                multi_camera->gimbal.pitch.max_angle,
                multi_camera->gimbal.pitch.max_speed);
            RCLCPP_DEBUG(node_->get_logger(), "          Yaw: enabled=%s, [%.3f, %.3f], max_speed=%.3f",
                multi_camera->gimbal.enable_yaw ? "true" : "false",
                multi_camera->gimbal.yaw.min_angle,
                multi_camera->gimbal.yaw.max_angle,
                multi_camera->gimbal.yaw.max_speed);
            RCLCPP_DEBUG(node_->get_logger(), "          Weight: %.3f", multi_camera->gimbal.weight);
            RCLCPP_DEBUG(node_->get_logger(), "          Power: idle=%.3f, active=%.3f",
                multi_camera->gimbal.idle_power, multi_camera->gimbal.active_power);
        }

        // Multi-Window Parameters
        for (const auto& [multi_window_id, multi_window] : agent->tracking.multi_window_set)
        {
            RCLCPP_DEBUG(node_->get_logger(), "      [MULTI-WINDOW: %s]", multi_window_id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Name: %s", multi_window->name.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Observation Set ID: %s", multi_window->observation_set_id.c_str());
            RCLCPP_DEBUG(node_->get_logger(), "        Resolution: %d x %d",
                multi_window->resolution(0), multi_window->resolution(1));
            RCLCPP_DEBUG(node_->get_logger(), "        Lambda: min=%.3f, max=%.3f, ref=%.3f",
                multi_window->min_lambda, multi_window->max_lambda, multi_window->ref_lambda);
        }

        // Drone Parameters
        RCLCPP_DEBUG(node_->get_logger(), "    [DRONE]");
        RCLCPP_DEBUG(node_->get_logger(), "      ID: %s", agent->drone.id.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "      Name: %s", agent->drone.name.c_str());
        RCLCPP_DEBUG(node_->get_logger(), "      Type: %s", droneTypeToString(agent->drone.type).c_str());
        RCLCPP_DEBUG(node_->get_logger(), "      Speed: cruise=%.3f, max=%.3f",
            agent->drone.cruise_speed, agent->drone.max_speed);
        RCLCPP_DEBUG(node_->get_logger(), "      Barometer: enabled=%s",
            agent->drone.enable_barometer ? "true" : "false");
        if (agent->drone.enable_barometer)
        {
            RCLCPP_DEBUG(node_->get_logger(), "        White Noise Sigma: %.6f", agent->drone.barometer.white_noise_sigma);
        }
        RCLCPP_DEBUG(node_->get_logger(), "      IMU: enabled=%s",
            agent->drone.enable_imu ? "true" : "false");
        if (agent->drone.enable_imu)
        {
            RCLCPP_DEBUG(node_->get_logger(), "        Angular White Noise Sigma: %.6f", agent->drone.imu.angular_white_noise_sigma);
            RCLCPP_DEBUG(node_->get_logger(), "        Velocity White Noise Sigma: %.6f", agent->drone.imu.velocity_white_noise_sigma);
        }
        RCLCPP_DEBUG(node_->get_logger(), "      GPS: enabled=%s",
            agent->drone.enable_gps ? "true" : "false");
        if (agent->drone.enable_gps)
        {
            RCLCPP_DEBUG(node_->get_logger(), "        EPH: initial=%.6f, final=%.6f",
                agent->drone.gps.eph_initial, agent->drone.gps.eph_final);
            RCLCPP_DEBUG(node_->get_logger(), "        EPV: initial=%.6f, final=%.6f",
                agent->drone.gps.epv_initial, agent->drone.gps.epv_final);
        }
        RCLCPP_DEBUG(node_->get_logger(), "      Magnetometer: enabled=%s",
            agent->drone.enable_magnetometer ? "true" : "false");
        if (agent->drone.enable_magnetometer)
        {
            RCLCPP_DEBUG(node_->get_logger(), "        White Noise Sigma: %.6f", agent->drone.magnetometer.white_noise_sigma);
            RCLCPP_DEBUG(node_->get_logger(), "        White Noise Bias: %.6f", agent->drone.magnetometer.white_noise_bias);
        }
        RCLCPP_DEBUG(node_->get_logger(), "      Weight: base=%.3f, max_payload=%.3f",
            agent->drone.base_weight, agent->drone.max_payload_weight);
        RCLCPP_DEBUG(node_->get_logger(), "      Power: hover=%.3f, cruise=%.3f",
            agent->drone.hover_power, agent->drone.cruise_power);
        RCLCPP_DEBUG(node_->get_logger(), "      Load Factor: %.3f", agent->drone.load_factor);
    }

    // System Parameters
    RCLCPP_DEBUG(node_->get_logger(), "\n[SYSTEM]");
    RCLCPP_DEBUG(node_->get_logger(), "  Simulation Framework: %s",
        simulationFrameworkToString(config_ptr_->system.simulation_framework).c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Clock Speed: %.3f", config_ptr_->system.clock_speed);
    RCLCPP_DEBUG(node_->get_logger(), "  Config Source File: %s", config_ptr_->system.config_source_file.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  AirSim Settings Destination File: %s", config_ptr_->system.airsim_settings_destination_file.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Trajectory Root: %s", config_ptr_->system.trajectory_root.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Scenario View ID: %s", config_ptr_->system.scenario_view_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Scenario Camera ID: %s", config_ptr_->system.scenario_camera_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Scenario Camera Position: [%.3f, %.3f, %.3f]",
        config_ptr_->system.scenario_camera_position.x(),
        config_ptr_->system.scenario_camera_position.y(),
        config_ptr_->system.scenario_camera_position.z());
    RCLCPP_DEBUG(node_->get_logger(), "  Agent View ID: %s", config_ptr_->system.agent_view_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Agent Camera ID: %s", config_ptr_->system.agent_camera_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Payload View ID: %s", config_ptr_->system.payload_view_id.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Payload Camera ID: %s", config_ptr_->system.payload_camera_id.c_str());

    // Topic Parameters
    RCLCPP_DEBUG(node_->get_logger(), "\n[TOPICS]");
    RCLCPP_DEBUG(node_->get_logger(), "  Coordinator Topics:");
    RCLCPP_DEBUG(node_->get_logger(), "    Registration: %s", config_ptr_->topics.registration.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Mission Status: %s", config_ptr_->topics.mission_status.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Fleet Status: %s", config_ptr_->topics.fleet_status.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Global Origin: %s", config_ptr_->topics.global_origin.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Target Position: %s", config_ptr_->topics.target_position.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Cluster Assignment: %s", config_ptr_->topics.cluster_assignment.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Cluster Geometry: %s", config_ptr_->topics.cluster_geometry.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Agent Assignment: %s", config_ptr_->topics.agent_assignment.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Agent Clusters: %s", config_ptr_->topics.agent_clusters.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Assignment Solve Duration: %s", config_ptr_->topics.assignment_solve_duration.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Assignment Node Count: %s", config_ptr_->topics.assignment_node_count.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Assignment Swap Count: %s", config_ptr_->topics.assignment_swap_count.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Agent Topics:");
    RCLCPP_DEBUG(node_->get_logger(), "    Status: %s", config_ptr_->topics.agent_status.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Global Position: %s", config_ptr_->topics.agent_global_position.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Local Position: %s", config_ptr_->topics.agent_local_position.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Position Setpoint: %s", config_ptr_->topics.agent_position_setpoint.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Position Solve Duration: %s", config_ptr_->topics.position_solve_duration.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Observation Setpoints: %s", config_ptr_->topics.observation_setpoints.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Image: %s", config_ptr_->topics.image.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Image Compressed: %s", config_ptr_->topics.image_compressed.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Camera Info: %s", config_ptr_->topics.camera_info.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Operator Topics:");
    RCLCPP_DEBUG(node_->get_logger(), "    Annotations: %s", config_ptr_->topics.annotations.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Scene: %s", config_ptr_->topics.scene.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Start Mission: %s", config_ptr_->topics.start_mission.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Pause Mission: %s", config_ptr_->topics.pause_mission.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Abort Mission: %s", config_ptr_->topics.abort_mission.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Arm All: %s", config_ptr_->topics.arm_all.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Land All: %s", config_ptr_->topics.land_all.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Return Home: %s", config_ptr_->topics.return_home.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Mission Metrics: %s", config_ptr_->topics.mission_metrics.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Fleet Metrics: %s", config_ptr_->topics.fleet_metrics.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Agent Metrics: %s", config_ptr_->topics.agent_metrics.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Target Metrics: %s", config_ptr_->topics.target_metrics.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Cluster Metrics: %s", config_ptr_->topics.cluster_metrics.c_str());

    // Frame Parameters
    RCLCPP_DEBUG(node_->get_logger(), "\n[FRAMES]");
    RCLCPP_DEBUG(node_->get_logger(), "  Global Frames:");
    RCLCPP_DEBUG(node_->get_logger(), "    World: %s", config_ptr_->frames.world.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "  Agent Frames:");
    RCLCPP_DEBUG(node_->get_logger(), "    Agent Local: %s", config_ptr_->frames.agent_local.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Agent Body: %s", config_ptr_->frames.agent_body.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Camera Body: %s", config_ptr_->frames.camera_body.c_str());
    RCLCPP_DEBUG(node_->get_logger(), "    Camera Optical: %s", config_ptr_->frames.camera_optical.c_str());

    RCLCPP_DEBUG(node_->get_logger(), "\n═══════════════════════════════════════════════════════════════");
}