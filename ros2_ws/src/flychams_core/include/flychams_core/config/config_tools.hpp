#pragma once

// Config includes
#include "flychams_core/config/spreadsheet_parser.hpp"
#include "flychams_core/config/airsim_settings_creator.hpp"
#include "flychams_core/config/agents_yaml_creator.hpp"

// Core includes
#include "flychams_core/utils/math_utils.hpp"
#include "flychams_core/utils/ros_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Config Manager for handling configuration parsing and
     * utilities.
     *
     * @details
     * This class provides utilities for managing the configuration of the
     * FlyChams system.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class ConfigTools
    {
    public: // Constructor/Destructor
        ConfigTools(NodePtr node)
            : node_(node)
        {
            // Parse the configuration spreadsheet
            const std::string& path = RosUtils::getParameter<std::string>(node_, "path.config_spreadsheet_path");
            try
            {
                RCLCPP_INFO(node_->get_logger(), "Parsing config spreadsheet: %s", path.c_str());
                config_ptr_ = SpreadsheetParser::parseSpreadsheet(path);
                RCLCPP_INFO(node_->get_logger(), "Config spreadsheet parsed successfully");
            }
            catch (const std::exception& e)
            {
                RCLCPP_ERROR(node_->get_logger(), "Error parsing config spreadsheet: %s", e.what());
                rclcpp::shutdown();
            }

            // Get parameters from node
            parseParameters(config_ptr_);
        }

        ~ConfigTools()
        {
            shutdown();
        }

        void shutdown()
        {
            // Destroy config
            config_ptr_.reset();
            // Destroy node
            node_.reset();
        }

    public: // Types
        using SharedPtr = std::shared_ptr<ConfigTools>;

    private: // Data
        // Configuration
        MissionConfigPtr config_ptr_;

        // ROS components
        NodePtr node_;

    public: // Settings utilities

        void createAirsimSettings() const
        {
            const std::string& path = RosUtils::getParameter<std::string>(node_, "path.airsim_settings_path");
            if (!AirsimSettingsCreator::createAirsimSettings(config_ptr_, path))
            {
                RCLCPP_ERROR(node_->get_logger(), "Failed to create AirSim settings.json at %s", path.c_str());
                rclcpp::shutdown();
            }
            else
            {
                RCLCPP_INFO(node_->get_logger(), "AirSim settings.json created successfully at %s", path.c_str());
            }
        }

        void createAgentsYaml() const
        {
            // Get path from parameters, default to config/agents.yaml if not specified
            std::string path;
            try
            {
                path = RosUtils::getParameter<std::string>(node_, "path.agents_yaml_path");
            }
            catch (const std::exception&)
            {
                // Default path relative to config spreadsheet
                const std::string& spreadsheet_path = RosUtils::getParameter<std::string>(node_, "path.config_spreadsheet_path");
                // Extract directory path manually
                size_t last_slash = spreadsheet_path.find_last_of("/\\");
                if (last_slash != std::string::npos)
                {
                    path = spreadsheet_path.substr(0, last_slash + 1) + "agents.yaml";
                }
                else
                {
                    path = "agents.yaml";
                }
            }

            if (!AgentsYamlCreator::createAgentsYaml(config_ptr_, path))
            {
                RCLCPP_ERROR(node_->get_logger(), "Failed to create agents.yaml at %s", path.c_str());
                rclcpp::shutdown();
            }
            else
            {
                RCLCPP_INFO(node_->get_logger(), "agents.yaml created successfully at %s", path.c_str());
            }
        }

    public: // Raw getter methods

        const MissionConfigPtr getConfig() const
        {
            return config_ptr_;
        }

        const EnvironmentConfig getEnvironment() const
        {
            return config_ptr_->environment;
        }

        const TargetGroupConfig getTargetGroup() const
        {
            return config_ptr_->target_group;
        }

        const TargetConfigPtr getTarget(const ID& target_id) const
        {
            return config_ptr_->target_group.at(target_id);
        }

        const AgentTeamConfig getAgentTeam() const
        {
            return config_ptr_->agent_team;
        }

        const AgentConfigPtr getAgent(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id);
        }

        const TrackingConfig getTracking(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking;
        }

        const MultiCameraSetConfig getMultiCameraSet(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set;
        }

        const MultiCameraConfigPtr getMultiCamera(const ID& agent_id, const ID& multi_camera_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set.at(multi_camera_id);
        }

        const MultiWindowSetConfig getMultiWindowSet(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_window_set;
        }

        const MultiWindowConfigPtr getMultiWindow(const ID& agent_id, const ID& multi_window_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_window_set.at(multi_window_id);
        }

        const DroneConfig getDrone(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->drone;
        }

        const CameraConfig getCamera(const ID& agent_id, const ID& multi_camera_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set.at(multi_camera_id)->camera;
        }

        const GimbalConfig getGimbal(const ID& agent_id, const ID& multi_camera_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set.at(multi_camera_id)->gimbal;
        }

        const SystemParameters getSystem() const
        {
            return config_ptr_->system;
        }

        const TopicParameters getTopics() const
        {
            return config_ptr_->topics;
        }

        const FrameParameters getFrames() const
        {
            return config_ptr_->frames;
        }

    public: // Processing getter methods
        const TrackingParameters getTrackingParameters(const std::string& agent_id) const
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

        const ObservationUnitParameters getObservationUnitParameters(const TrackingConfig& tracking, const MultiCameraConfigPtr& multi_camera) const
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

        const ObservationUnitParameters getObservationUnitParameters(const TrackingConfig& tracking, const ObservationUnitParameters& central_camera_params, const MultiWindowConfigPtr& multi_window) const
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

    private: // Parameter parsing
        void parseParameters(MissionConfigPtr& config_ptr)
        {
            // Parse system, topics and frames parameters
            parseSystemParameters(config_ptr);
            parseTopicParameters(config_ptr);
            parseFrameParameters(config_ptr);
        }

        void parseSystemParameters(MissionConfigPtr& config_ptr)
        {
            // Simulation settings
            const std::string& simulation_framework_str = RosUtils::getParameter<std::string>(node_, "simulation.framework");
            config_ptr->system.simulation_framework = simulationFrameworkFromString(simulation_framework_str);
            config_ptr->system.clock_speed = RosUtils::getParameter<float>(node_, "simulation.clock_speed");

            // Path settings
            config_ptr->system.config_source_file = RosUtils::getParameter<std::string>(node_, "path.config_spreadsheet_path");
            config_ptr->system.airsim_settings_destination_file = RosUtils::getParameter<std::string>(node_, "path.airsim_settings_path");
            config_ptr->system.trajectory_root = RosUtils::getParameter<std::string>(node_, "path.trajectory_root");

            // GUI settings
            // Scenario view settings
            config_ptr->system.scenario_view_id = RosUtils::getParameter<ID>(node_, "gui.scenario_view_id");
            config_ptr->system.scenario_camera_id = RosUtils::getParameter<ID>(node_, "gui.scenario_camera_id");
            const std::vector<double> scenario_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.scenario_camera_position");
            if (scenario_camera_position_vec.size() >= 3)
            {
                config_ptr->system.scenario_camera_position.x() = scenario_camera_position_vec[0];
                config_ptr->system.scenario_camera_position.y() = scenario_camera_position_vec[1];
                config_ptr->system.scenario_camera_position.z() = scenario_camera_position_vec[2];
            }
            const std::vector<double> scenario_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.scenario_camera_orientation");
            if (scenario_camera_orientation_vec.size() >= 3)
            {
                config_ptr->system.scenario_camera_orientation.x() = MathUtils::degToRad(scenario_camera_orientation_vec[0]);
                config_ptr->system.scenario_camera_orientation.y() = MathUtils::degToRad(scenario_camera_orientation_vec[1]);
                config_ptr->system.scenario_camera_orientation.z() = MathUtils::degToRad(scenario_camera_orientation_vec[2]);
            }
            // Agent view settings
            config_ptr->system.agent_view_id = RosUtils::getParameter<ID>(node_, "gui.agent_view_id");
            config_ptr->system.agent_camera_id = RosUtils::getParameter<ID>(node_, "gui.agent_camera_id");
            const std::vector<double> agent_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.agent_camera_position");
            if (agent_camera_position_vec.size() >= 3)
            {
                config_ptr->system.agent_camera_position.x() = agent_camera_position_vec[0];
                config_ptr->system.agent_camera_position.y() = agent_camera_position_vec[1];
                config_ptr->system.agent_camera_position.z() = agent_camera_position_vec[2];
            }
            const std::vector<double> agent_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.agent_camera_orientation");
            if (agent_camera_orientation_vec.size() >= 3)
            {
                config_ptr->system.agent_camera_orientation.x() = MathUtils::degToRad(agent_camera_orientation_vec[0]);
                config_ptr->system.agent_camera_orientation.y() = MathUtils::degToRad(agent_camera_orientation_vec[1]);
                config_ptr->system.agent_camera_orientation.z() = MathUtils::degToRad(agent_camera_orientation_vec[2]);
            }
            // Payload view settings
            config_ptr->system.payload_view_id = RosUtils::getParameter<ID>(node_, "gui.payload_view_id");
            config_ptr->system.payload_camera_id = RosUtils::getParameter<ID>(node_, "gui.payload_camera_id");
            const std::vector<double> payload_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.payload_camera_position");
            if (payload_camera_position_vec.size() >= 3)
            {
                config_ptr->system.payload_camera_position.x() = payload_camera_position_vec[0];
                config_ptr->system.payload_camera_position.y() = payload_camera_position_vec[1];
                config_ptr->system.payload_camera_position.z() = payload_camera_position_vec[2];
            }
            const std::vector<double> payload_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.payload_camera_orientation");
            if (payload_camera_orientation_vec.size() >= 3)
            {
                config_ptr->system.payload_camera_orientation.x() = MathUtils::degToRad(payload_camera_orientation_vec[0]);
                config_ptr->system.payload_camera_orientation.y() = MathUtils::degToRad(payload_camera_orientation_vec[1]);
                config_ptr->system.payload_camera_orientation.z() = MathUtils::degToRad(payload_camera_orientation_vec[2]);
            }
            // Map view settings
            config_ptr->system.map_view_id = RosUtils::getParameter<ID>(node_, "gui.map_view_id");
            config_ptr->system.map_camera_id = RosUtils::getParameter<ID>(node_, "gui.map_camera_id");
            const std::vector<double> map_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.map_camera_position");
            if (map_camera_position_vec.size() >= 3)
            {
                config_ptr->system.map_camera_position.x() = map_camera_position_vec[0];
                config_ptr->system.map_camera_position.y() = map_camera_position_vec[1];
                config_ptr->system.map_camera_position.z() = map_camera_position_vec[2];
            }
            const std::vector<double> map_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node_, "gui.map_camera_orientation");
            if (map_camera_orientation_vec.size() >= 3)
            {
                config_ptr->system.map_camera_orientation.x() = MathUtils::degToRad(map_camera_orientation_vec[0]);
                config_ptr->system.map_camera_orientation.y() = MathUtils::degToRad(map_camera_orientation_vec[1]);
                config_ptr->system.map_camera_orientation.z() = MathUtils::degToRad(map_camera_orientation_vec[2]);
            }
            // Tracking views settings
            config_ptr->system.tracking_view_ids = RosUtils::getParameter<std::vector<ID>>(node_, "gui.tracking_view_ids");
        }

        void parseTopicParameters(MissionConfigPtr& config_ptr)
        {
            // Global topics
            config_ptr->topics.registration = RosUtils::getParameter<std::string>(node_, "global_topics.registration");
            config_ptr->topics.global_origin = RosUtils::getParameter<std::string>(node_, "global_topics.global_origin");
            config_ptr->topics.global_metrics = RosUtils::getParameter<std::string>(node_, "global_topics.metrics");

            // Agent topics
            config_ptr->topics.agent_status = RosUtils::getParameter<std::string>(node_, "agent_topics.status");
            config_ptr->topics.agent_position = RosUtils::getParameter<std::string>(node_, "agent_topics.position");
            config_ptr->topics.agent_assignment = RosUtils::getParameter<std::string>(node_, "agent_topics.assignment");
            config_ptr->topics.agent_clusters = RosUtils::getParameter<std::string>(node_, "agent_topics.clusters");
            config_ptr->topics.agent_position_setpoint = RosUtils::getParameter<std::string>(node_, "agent_topics.position_setpoint");
            config_ptr->topics.agent_optimization_duration = RosUtils::getParameter<std::string>(node_, "agent_topics.optimization_duration");
            config_ptr->topics.agent_observation_setpoints = RosUtils::getParameter<std::string>(node_, "agent_topics.observation_setpoints");
            config_ptr->topics.agent_metrics = RosUtils::getParameter<std::string>(node_, "agent_topics.metrics");
            config_ptr->topics.agent_markers = RosUtils::getParameter<std::string>(node_, "agent_topics.markers");

            // Target topics
            config_ptr->topics.target_true_position = RosUtils::getParameter<std::string>(node_, "target_topics.true_position");
            config_ptr->topics.target_est_position = RosUtils::getParameter<std::string>(node_, "target_topics.est_position");
            config_ptr->topics.target_metrics = RosUtils::getParameter<std::string>(node_, "target_topics.metrics");
            config_ptr->topics.target_markers = RosUtils::getParameter<std::string>(node_, "target_topics.markers");

            // Cluster topics
            config_ptr->topics.cluster_assignment = RosUtils::getParameter<std::string>(node_, "cluster_topics.assignment");
            config_ptr->topics.cluster_geometry = RosUtils::getParameter<std::string>(node_, "cluster_topics.geometry");
            config_ptr->topics.cluster_metrics = RosUtils::getParameter<std::string>(node_, "cluster_topics.metrics");
            config_ptr->topics.cluster_markers = RosUtils::getParameter<std::string>(node_, "cluster_topics.markers");

            // GUI topics
            config_ptr->topics.gui_setpoints = RosUtils::getParameter<std::string>(node_, "gui_topics.setpoints");
        }

        void parseFrameParameters(MissionConfigPtr& config_ptr)
        {
            // Global frames
            config_ptr->frames.world = RosUtils::getParameter<std::string>(node_, "global_frames.world");

            // Agent frames
            config_ptr->frames.agent_local = RosUtils::getParameter<std::string>(node_, "agent_frames.agent_local");
            config_ptr->frames.agent_body = RosUtils::getParameter<std::string>(node_, "agent_frames.agent_body");
            config_ptr->frames.camera_body = RosUtils::getParameter<std::string>(node_, "agent_frames.camera_body");
            config_ptr->frames.camera_optical = RosUtils::getParameter<std::string>(node_, "agent_frames.camera_optical");

        }
    };

} // namespace flychams::core