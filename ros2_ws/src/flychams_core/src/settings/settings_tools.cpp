#include "flychams_core/settings/settings_tools.hpp"

namespace flychams::core
{
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
        RCLCPP_INFO(node_->get_logger(), "------ Observation unit parameters -------");
        RCLCPP_INFO(node_->get_logger(), "  Unit ID: %s", params.id.c_str());
        RCLCPP_INFO(node_->get_logger(), "  Unit type: %s", observationTypeToString(params.type).c_str());
        RCLCPP_INFO(node_->get_logger(), "  Unit role: %s", observationRoleToString(params.role).c_str());
        RCLCPP_INFO(node_->get_logger(), "  Zoom factor limits: min=%.3f, max=%.3f, ref=%.3f [m]", params.upsilon_min, params.upsilon_max, params.upsilon_ref);
        RCLCPP_INFO(node_->get_logger(), "  Regularized pixel size: %.6f [m/pix]", params.rho);
        RCLCPP_INFO(node_->get_logger(), "  Target size limits: min=%.2f [pix], max=%.2f [pix], ref=%.2f [pix]", params.s_min_pix, params.s_max_pix, params.s_ref_pix);
        RCLCPP_INFO(node_->get_logger(), "  Camera resolution: %d x %d [pix]", params.camera_params.width, params.camera_params.height);
        RCLCPP_INFO(node_->get_logger(), "  Sensor dimensions: %.6f x %.6f [m]", params.camera_params.sensor_width, params.camera_params.sensor_height);
        RCLCPP_INFO(node_->get_logger(), "  Intrinsic matrix K: fx=%f fy=%f cx=%f cy=%f", params.camera_params.K(0, 0), params.camera_params.K(1, 1), params.camera_params.K(0, 2), params.camera_params.K(1, 2));
        RCLCPP_INFO(node_->get_logger(), "----------------------------------------");

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

        // Window resolution factor limits
        params.upsilon_min = multi_window->min_lambda;
        params.upsilon_max = multi_window->max_lambda;
        params.upsilon_ref = multi_window->ref_lambda;

        // Get central camera parameters
        params.camera_params = central_camera_params.camera_params;

        // Get window parameters
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
        RCLCPP_INFO(node_->get_logger(), "------ Observation unit parameters -------");
        RCLCPP_INFO(node_->get_logger(), "  Unit ID: %s", params.id.c_str());
        RCLCPP_INFO(node_->get_logger(), "  Unit type: %s", observationTypeToString(params.type).c_str());
        RCLCPP_INFO(node_->get_logger(), "  Unit role: %s", observationRoleToString(params.role).c_str());
        RCLCPP_INFO(node_->get_logger(), "  Zoom factor limits: min=%.3f, max=%.3f, ref=%.3f [m]", params.upsilon_min, params.upsilon_max, params.upsilon_ref);
        RCLCPP_INFO(node_->get_logger(), "  Regularized pixel size: %.6f [m/pix]", params.rho);
        RCLCPP_INFO(node_->get_logger(), "  Target size limits: min=%.2f [pix], max=%.2f [pix], ref=%.2f [pix]", params.s_min_pix, params.s_max_pix, params.s_ref_pix);
        RCLCPP_INFO(node_->get_logger(), "  Window full resolution: %d x %d [pix]", params.window_params.full_width, params.window_params.full_height);
        RCLCPP_INFO(node_->get_logger(), "  Window tracking resolution: %d x %d [pix]", params.window_params.tracking_width, params.window_params.tracking_height);
        RCLCPP_INFO(node_->get_logger(), "----------------------------------------");

        return params;
    }

} // namespace flychams::core