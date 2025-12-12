#pragma once

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/config_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/math_utils.hpp"
#include "flychams_core/utils/ros_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Config Manager for getting configuration parameters from the
     * ROS2 parameters server.
     *
     * @details
     * This class provides utilities for getting configuration parameters
     * from the ROS2 parameters server.
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
            // Initialize config pointer
            config_ptr_ = std::make_shared<MissionConfig>();

            // Set prefixes for parameters (matching config_parser.cpp)
            std::string mission_prefix = "mission.";
            std::string environment_prefix = "environment.";
            std::string targets_prefix = "targets.";
            std::string agents_prefix = "agents.";

            // Parse mission parameters from ROS2 parameters server
            parseMissionParameters(mission_prefix, config_ptr_);
            parseEnvironmentParameters(environment_prefix, config_ptr_);
            parseTargetParameters(targets_prefix, config_ptr_);
            parseAgentParameters(agents_prefix, config_ptr_);

            // Parse system, topics and frames parameters from ROS2 parameters server
            parseSystemParameters(config_ptr_);
            parseTopicParameters(config_ptr_);
            parseFrameParameters(config_ptr_);
        }

        ~ConfigTools()
        {
            shutdown();
        }

        void shutdown()
        {
            // Nothing to do
        }

    public: // Types
        using SharedPtr = std::shared_ptr<ConfigTools>;

    private: // Data
        // Configuration
        MissionConfigPtr config_ptr_;

        // ROS components
        NodePtr node_;

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
        void parseMissionParameters(const std::string& prefix, MissionConfigPtr& config_ptr)
        {
            config_ptr->id = RosUtils::getParameter<std::string>(node_, prefix + "id");
            config_ptr->name = RosUtils::getParameter<std::string>(node_, prefix + "name");

            config_ptr->environment_id = RosUtils::getParameter<std::string>(node_, prefix + "environment_id");
            config_ptr->target_group_id = RosUtils::getParameter<std::string>(node_, prefix + "target_group_id");
            config_ptr->agent_team_id = RosUtils::getParameter<std::string>(node_, prefix + "agent_team_id"); 

            config_ptr->horizontal_constraint(0) = RosUtils::getParameter<float>(node_, prefix + "horizontal_constraint.min");
            config_ptr->horizontal_constraint(1) = RosUtils::getParameter<float>(node_, prefix + "horizontal_constraint.max");

            config_ptr->vertical_constraint(0) = RosUtils::getParameter<float>(node_, prefix + "vertical_constraint.min");
            config_ptr->vertical_constraint(1) = RosUtils::getParameter<float>(node_, prefix + "vertical_constraint.max");
            
            const int8_t autopilot_int = RosUtils::getParameter<int8_t>(node_, prefix + "autopilot");
            config_ptr->autopilot = static_cast<Autopilot>(autopilot_int);
            
            config_ptr->start_date.year = RosUtils::getParameter<int64_t>(node_, prefix + "start_date.year");
            config_ptr->start_date.month = RosUtils::getParameter<int64_t>(node_, prefix + "start_date.month");
            config_ptr->start_date.day = RosUtils::getParameter<int64_t>(node_, prefix + "start_date.day");
            
            config_ptr->start_hour.hours = RosUtils::getParameter<int64_t>(node_, prefix + "start_hour.hours");
            config_ptr->start_hour.minutes = RosUtils::getParameter<int64_t>(node_, prefix + "start_hour.minutes");
            config_ptr->start_hour.seconds = RosUtils::getParameter<int64_t>(node_, prefix + "start_hour.seconds");
        }

        void parseEnvironmentParameters(const std::string& prefix, MissionConfigPtr& config_ptr)
        {
            config_ptr->environment.id = RosUtils::getParameter<std::string>(node_, prefix + "id");
            config_ptr->environment.name = RosUtils::getParameter<std::string>(node_, prefix + "name");
            
            config_ptr->environment.geopoint.latitude = RosUtils::getParameter<double>(node_, prefix + "geopoint.latitude");
            config_ptr->environment.geopoint.longitude = RosUtils::getParameter<double>(node_, prefix + "geopoint.longitude");
            config_ptr->environment.geopoint.altitude = RosUtils::getParameter<double>(node_, prefix + "geopoint.altitude");
            
            config_ptr->environment.wind_vel.x() = RosUtils::getParameter<float>(node_, prefix + "wind_vel.x");
            config_ptr->environment.wind_vel.y() = RosUtils::getParameter<float>(node_, prefix + "wind_vel.y");
            config_ptr->environment.wind_vel.z() = RosUtils::getParameter<float>(node_, prefix + "wind_vel.z");
        }

        void parseTargetParameters(const std::string& prefix, MissionConfigPtr& config_ptr)
        {
            // Get target ID list and iterate over them
            std::vector<std::string> target_ids = RosUtils::getParameter<std::vector<std::string>>(node_, prefix + "id_list");
            for (const auto& target_id : target_ids)
            {
                auto target = std::make_shared<TargetConfig>();
                target->id = target_id;
                const std::string target_prefix = prefix + target_id + ".";
                
                target->name = RosUtils::getParameter<std::string>(node_, target_prefix + "name");
                target->target_group_id = RosUtils::getParameter<std::string>(node_, target_prefix + "target_group_id");
                target->target_index = RosUtils::getParameter<int>(node_, target_prefix + "target_index");
                
                const int8_t type_int = RosUtils::getParameter<int8_t>(node_, target_prefix + "type");
                target->type = static_cast<TargetType>(type_int);
                
                target->count = RosUtils::getParameter<int>(node_, target_prefix + "count");
                
                const int8_t priority_int = RosUtils::getParameter<int8_t>(node_, target_prefix + "priority");
                target->priority = static_cast<Priority>(priority_int);
                
                target->trajectory_folder = RosUtils::getParameter<std::string>(node_, target_prefix + "trajectory_folder");
                
                config_ptr->target_group[target_id] = target;
            }
        }

        void parseAgentParameters(const std::string& prefix, MissionConfigPtr& config_ptr)
        {
            // Get agent ID list and iterate over them
            std::vector<std::string> agent_ids = RosUtils::getParameter<std::vector<std::string>>(node_, prefix + "id_list");
            for (const auto& agent_id : agent_ids)
            {
                auto agent = std::make_shared<AgentConfig>();
                agent->id = agent_id;
                const std::string agent_prefix = prefix + agent_id + ".";
                
                agent->name = RosUtils::getParameter<std::string>(node_, agent_prefix + "name");
                agent->agent_team_id = RosUtils::getParameter<std::string>(node_, agent_prefix + "agent_team_id");
                agent->tracking_id = RosUtils::getParameter<std::string>(node_, agent_prefix + "tracking_id");
                agent->drone_id = RosUtils::getParameter<std::string>(node_, agent_prefix + "drone_id");
                
                // Parse position (x, y, z)
                agent->position.x() = RosUtils::getParameter<double>(node_, agent_prefix + "position.x");
                agent->position.y() = RosUtils::getParameter<double>(node_, agent_prefix + "position.y");
                agent->position.z() = RosUtils::getParameter<double>(node_, agent_prefix + "position.z");
                
                // Parse orientation (roll, pitch, yaw)
                agent->orientation.x() = RosUtils::getParameter<double>(node_, agent_prefix + "orientation.roll");
                agent->orientation.y() = RosUtils::getParameter<double>(node_, agent_prefix + "orientation.pitch");
                agent->orientation.z() = RosUtils::getParameter<double>(node_, agent_prefix + "orientation.yaw");
                
                agent->safety_radius = RosUtils::getParameter<float>(node_, agent_prefix + "safety_radius");
                agent->max_altitude = RosUtils::getParameter<float>(node_, agent_prefix + "max_altitude");
                agent->battery_capacity = RosUtils::getParameter<float>(node_, agent_prefix + "battery_capacity");
                
                parseTrackingParameters(agent, agent_prefix + "tracking.");
                parseDroneParameters(agent, agent_prefix + "drone.");
                
                config_ptr->agent_team[agent_id] = agent;
            }
        }

        void parseTrackingParameters(AgentConfigPtr& agent, const std::string& prefix)
        {
            agent->tracking.id = RosUtils::getParameter<std::string>(node_, prefix + "id");
            agent->tracking.name = RosUtils::getParameter<std::string>(node_, prefix + "name");
            agent->tracking.observation_set_id = RosUtils::getParameter<std::string>(node_, prefix + "observation_set_id");
            agent->tracking.min_target_size = RosUtils::getParameter<float>(node_, prefix + "min_target_size");
            agent->tracking.max_target_size = RosUtils::getParameter<float>(node_, prefix + "max_target_size");
            agent->tracking.ref_target_size = RosUtils::getParameter<float>(node_, prefix + "ref_target_size");
            
            // Get multi camera ID list and iterate over them
            std::vector<std::string> multi_camera_ids = RosUtils::getParameter<std::vector<std::string>>(node_, prefix + "multi_cameras.ids");
            for (const auto& multi_camera_id : multi_camera_ids)
            {
                auto multi_camera = std::make_shared<MultiCameraConfig>();
                multi_camera->id = multi_camera_id;
                parseMultiCameraParameters(multi_camera, prefix + "multi_cameras." + multi_camera_id + ".");
                agent->tracking.multi_camera_set[multi_camera_id] = multi_camera;
            }
            
            // Get multi window ID list and iterate over them
            std::vector<std::string> multi_window_ids = RosUtils::getParameter<std::vector<std::string>>(node_, prefix + "multi_windows.ids");
            for (const auto& multi_window_id : multi_window_ids)
            {
                auto multi_window = std::make_shared<MultiWindowConfig>();
                multi_window->id = multi_window_id;
                parseMultiWindowParameters(multi_window, prefix + "multi_windows." + multi_window_id + ".");
                agent->tracking.multi_window_set[multi_window_id] = multi_window;
            }
        }

        void parseMultiCameraParameters(MultiCameraConfigPtr& multi_camera, const std::string& prefix)
        {
            multi_camera->name = RosUtils::getParameter<std::string>(node_, prefix + "name");
            multi_camera->observation_set_id = RosUtils::getParameter<std::string>(node_, prefix + "observation_set_id");
            multi_camera->camera_id = RosUtils::getParameter<std::string>(node_, prefix + "camera_id");
            multi_camera->gimbal_id = RosUtils::getParameter<std::string>(node_, prefix + "gimbal_id");
            
            const int8_t role_int = RosUtils::getParameter<int8_t>(node_, prefix + "role");
            multi_camera->role = static_cast<ObservationRole>(role_int);
            
            // Parse position (x, y, z)
            multi_camera->position.x() = RosUtils::getParameter<double>(node_, prefix + "position.x");
            multi_camera->position.y() = RosUtils::getParameter<double>(node_, prefix + "position.y");
            multi_camera->position.z() = RosUtils::getParameter<double>(node_, prefix + "position.z");
            
            // Parse orientation (roll, pitch, yaw)
            multi_camera->orientation.x() = RosUtils::getParameter<double>(node_, prefix + "orientation.roll");
            multi_camera->orientation.y() = RosUtils::getParameter<double>(node_, prefix + "orientation.pitch");
            multi_camera->orientation.z() = RosUtils::getParameter<double>(node_, prefix + "orientation.yaw");
            
            multi_camera->min_focal = RosUtils::getParameter<float>(node_, prefix + "min_focal");
            multi_camera->max_focal = RosUtils::getParameter<float>(node_, prefix + "max_focal");
            multi_camera->ref_focal = RosUtils::getParameter<float>(node_, prefix + "ref_focal");
            
            parseCameraParameters(multi_camera, prefix + "camera.");
            parseGimbalParameters(multi_camera, prefix + "gimbal.");
        }

        void parseMultiWindowParameters(MultiWindowConfigPtr& multi_window, const std::string& prefix)
        {
            multi_window->name = RosUtils::getParameter<std::string>(node_, prefix + "name");
            multi_window->observation_set_id = RosUtils::getParameter<std::string>(node_, prefix + "observation_set_id");
            
            // Parse resolution (width, height)
            multi_window->resolution(0) = RosUtils::getParameter<int64_t>(node_, prefix + "resolution.width");
            multi_window->resolution(1) = RosUtils::getParameter<int64_t>(node_, prefix + "resolution.height");
            
            multi_window->min_lambda = RosUtils::getParameter<float>(node_, prefix + "min_lambda");
            multi_window->max_lambda = RosUtils::getParameter<float>(node_, prefix + "max_lambda");
            multi_window->ref_lambda = RosUtils::getParameter<float>(node_, prefix + "ref_lambda");
        }

        void parseCameraParameters(MultiCameraConfigPtr& multi_camera, const std::string& prefix)
        {
            multi_camera->camera.id = RosUtils::getParameter<std::string>(node_, prefix + "id");
            multi_camera->camera.name = RosUtils::getParameter<std::string>(node_, prefix + "name");
            
            const int8_t type_int = RosUtils::getParameter<int8_t>(node_, prefix + "type");
            multi_camera->camera.type = static_cast<CameraType>(type_int);
            
            // Parse resolution (width, height)
            multi_camera->camera.resolution(0) = RosUtils::getParameter<int64_t>(node_, prefix + "resolution.width");
            multi_camera->camera.resolution(1) = RosUtils::getParameter<int64_t>(node_, prefix + "resolution.height");
            
            // Parse sensor_size (width, height)
            multi_camera->camera.sensor_size(0) = RosUtils::getParameter<double>(node_, prefix + "sensor_size.width");
            multi_camera->camera.sensor_size(1) = RosUtils::getParameter<double>(node_, prefix + "sensor_size.height");
            
            // Parse distortion (K1, K2, K3, P1, P2)
            multi_camera->camera.distortion.K1 = RosUtils::getParameter<double>(node_, prefix + "distortion.K1");
            multi_camera->camera.distortion.K2 = RosUtils::getParameter<double>(node_, prefix + "distortion.K2");
            multi_camera->camera.distortion.K3 = RosUtils::getParameter<double>(node_, prefix + "distortion.K3");
            multi_camera->camera.distortion.P1 = RosUtils::getParameter<double>(node_, prefix + "distortion.P1");
            multi_camera->camera.distortion.P2 = RosUtils::getParameter<double>(node_, prefix + "distortion.P2");
            
            multi_camera->camera.enable_sensor_noise = RosUtils::getParameter<bool>(node_, prefix + "enable_sensor_noise");
            
            // Parse sensor_noise (rand_contrib, rand_size, rand_speed)
            multi_camera->camera.sensor_noise.rand_contrib = RosUtils::getParameter<double>(node_, prefix + "sensor_noise.rand_contrib");
            multi_camera->camera.sensor_noise.rand_size = RosUtils::getParameter<double>(node_, prefix + "sensor_noise.rand_size");
            multi_camera->camera.sensor_noise.rand_speed = RosUtils::getParameter<double>(node_, prefix + "sensor_noise.rand_speed");
            
            multi_camera->camera.weight = RosUtils::getParameter<float>(node_, prefix + "weight");
            multi_camera->camera.idle_power = RosUtils::getParameter<float>(node_, prefix + "idle_power");
            multi_camera->camera.active_power = RosUtils::getParameter<float>(node_, prefix + "active_power");
        }

        void parseGimbalParameters(MultiCameraConfigPtr& multi_camera, const std::string& prefix)
        {
            multi_camera->gimbal.id = RosUtils::getParameter<std::string>(node_, prefix + "id");
            multi_camera->gimbal.name = RosUtils::getParameter<std::string>(node_, prefix + "name");
            multi_camera->gimbal.enable_roll = RosUtils::getParameter<bool>(node_, prefix + "enable_roll");
            
            // Parse roll (min_angle, max_angle, max_speed)
            multi_camera->gimbal.roll.min_angle = RosUtils::getParameter<double>(node_, prefix + "roll.min_angle");
            multi_camera->gimbal.roll.max_angle = RosUtils::getParameter<double>(node_, prefix + "roll.max_angle");
            multi_camera->gimbal.roll.max_speed = RosUtils::getParameter<double>(node_, prefix + "roll.max_speed");
            
            multi_camera->gimbal.enable_pitch = RosUtils::getParameter<bool>(node_, prefix + "enable_pitch");
            
            // Parse pitch (min_angle, max_angle, max_speed)
            multi_camera->gimbal.pitch.min_angle = RosUtils::getParameter<double>(node_, prefix + "pitch.min_angle");
            multi_camera->gimbal.pitch.max_angle = RosUtils::getParameter<double>(node_, prefix + "pitch.max_angle");
            multi_camera->gimbal.pitch.max_speed = RosUtils::getParameter<double>(node_, prefix + "pitch.max_speed");
            
            multi_camera->gimbal.enable_yaw = RosUtils::getParameter<bool>(node_, prefix + "enable_yaw");
            
            // Parse yaw (min_angle, max_angle, max_speed)
            multi_camera->gimbal.yaw.min_angle = RosUtils::getParameter<double>(node_, prefix + "yaw.min_angle");
            multi_camera->gimbal.yaw.max_angle = RosUtils::getParameter<double>(node_, prefix + "yaw.max_angle");
            multi_camera->gimbal.yaw.max_speed = RosUtils::getParameter<double>(node_, prefix + "yaw.max_speed");
            
            multi_camera->gimbal.weight = RosUtils::getParameter<float>(node_, prefix + "weight");
            multi_camera->gimbal.idle_power = RosUtils::getParameter<float>(node_, prefix + "idle_power");
            multi_camera->gimbal.active_power = RosUtils::getParameter<float>(node_, prefix + "active_power");
        }

        void parseDroneParameters(AgentConfigPtr& agent, const std::string& prefix)
        {
            agent->drone.id = RosUtils::getParameter<std::string>(node_, prefix + "id");
            agent->drone.name = RosUtils::getParameter<std::string>(node_, prefix + "name");
            
            const int8_t type_int = RosUtils::getParameter<int8_t>(node_, prefix + "type");
            agent->drone.type = static_cast<DroneType>(type_int);
            
            agent->drone.cruise_speed = RosUtils::getParameter<float>(node_, prefix + "cruise_speed");
            agent->drone.max_speed = RosUtils::getParameter<float>(node_, prefix + "max_speed");
            
            agent->drone.enable_barometer = RosUtils::getParameter<bool>(node_, prefix + "enable_barometer");
            // Parse barometer (white_noise_sigma)
            agent->drone.barometer.white_noise_sigma = RosUtils::getParameter<double>(node_, prefix + "barometer.white_noise_sigma");
            
            agent->drone.enable_imu = RosUtils::getParameter<bool>(node_, prefix + "enable_imu");
            // Parse imu (angular_white_noise_sigma, velocity_white_noise_sigma)
            agent->drone.imu.angular_white_noise_sigma = RosUtils::getParameter<double>(node_, prefix + "imu.angular_white_noise_sigma");
            agent->drone.imu.velocity_white_noise_sigma = RosUtils::getParameter<double>(node_, prefix + "imu.velocity_white_noise_sigma");
            
            agent->drone.enable_gps = RosUtils::getParameter<bool>(node_, prefix + "enable_gps");
            // Parse gps (eph_initial, epv_initial, eph_final, epv_final)
            agent->drone.gps.eph_initial = RosUtils::getParameter<double>(node_, prefix + "gps.eph_initial");
            agent->drone.gps.epv_initial = RosUtils::getParameter<double>(node_, prefix + "gps.epv_initial");
            agent->drone.gps.eph_final = RosUtils::getParameter<double>(node_, prefix + "gps.eph_final");
            agent->drone.gps.epv_final = RosUtils::getParameter<double>(node_, prefix + "gps.epv_final");
            
            agent->drone.enable_magnetometer = RosUtils::getParameter<bool>(node_, prefix + "enable_magnetometer");
            // Parse magnetometer (white_noise_sigma, white_noise_bias)
            agent->drone.magnetometer.white_noise_sigma = RosUtils::getParameter<double>(node_, prefix + "magnetometer.white_noise_sigma");
            agent->drone.magnetometer.white_noise_bias = RosUtils::getParameter<double>(node_, prefix + "magnetometer.white_noise_bias");
            
            agent->drone.base_weight = RosUtils::getParameter<float>(node_, prefix + "base_weight");
            agent->drone.max_payload_weight = RosUtils::getParameter<float>(node_, prefix + "max_payload_weight");
            agent->drone.hover_power = RosUtils::getParameter<float>(node_, prefix + "hover_power");
            agent->drone.cruise_power = RosUtils::getParameter<float>(node_, prefix + "cruise_power");
            agent->drone.load_factor = RosUtils::getParameter<float>(node_, prefix + "load_factor");
        }

        void parseSystemParameters(MissionConfigPtr& config_ptr)
        {
            // Simulation settings
            const std::string simulation_framework_str = RosUtils::getParameter<std::string>(node_, "simulation.framework");
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
            config_ptr->topics.agent_local_position = RosUtils::getParameter<std::string>(node_, "agent_topics.local_position");
            config_ptr->topics.agent_global_position = RosUtils::getParameter<std::string>(node_, "agent_topics.global_position");
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