#pragma once

// Standard includes
#include <string>
#include <vector>
#include <map>
#include <memory>

// Core includes
#include "flychams_common/types/core_types.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Types for mission configuration
     *
     * @details
     * This file contains all the configuration data structures used
     * to define mission settings, including hardware types, mission
     * types, and other types.
     * ════════════════════════════════════════════════════════════════
     */

    // ════════════════════════════════════════════════════════════════
    // OTHER TYPES: Other configuration types
    // ════════════════════════════════════════════════════════════════

    struct SystemParameters
    {
        // Simulation settings
        SimulationFramework simulation_framework;
        float clock_speed;
        int quality_preset;

        // Path settings
        std::string config_source_file;
        std::string airsim_settings_destination_file;
        std::string trajectory_root;

        // GUI settings
        // Scenario view settings
        ID scenario_view_id;
        ID scenario_camera_id;
        Vector3r scenario_camera_position;
        Vector3r scenario_camera_orientation;
        // Agent view settings
        ID agent_view_id;
        ID agent_camera_id;
        Vector3r agent_camera_position;
        Vector3r agent_camera_orientation;
        // Payload view settings
        ID payload_view_id;
        ID payload_camera_id;
        Vector3r payload_camera_position;
        Vector3r payload_camera_orientation;
        // Map view settings
        ID map_view_id;
        ID map_camera_id;
        Vector3r map_camera_position;
        Vector3r map_camera_orientation;
        // Tracking views settings
        std::vector<ID> tracking_view_ids;
    };

    struct TopicParameters
    {
        // Coordinator topics
        std::string registration;
        std::string global_origin;
        std::string target_position;
        std::string cluster_assignment;
        std::string cluster_geometry;

        // Agent topics
        std::string agent_status;
        std::string agent_global_position;
        std::string agent_local_position;
        std::string agent_assignment;
        std::string agent_clusters;
        std::string agent_position_setpoint;
        std::string agent_observation_setpoints;
        std::string agent_multi_camera_image;
        std::string agent_multi_window_image;

        // Operator topics
        std::string mission_metrics;
        std::string agent_metrics;
        std::string target_metrics;
        std::string cluster_metrics;
        std::string agent_markers;
        std::string target_markers;
        std::string cluster_markers;
    };

    struct FrameParameters
    {
        // Global frames
        std::string world;

        // Agent frames
        std::string agent_local;
        std::string agent_body;
        std::string camera_body;
        std::string camera_optical;
    };

    // ════════════════════════════════════════════════════════════════
    // HARDWARE TYPES: Hardware-related configuration types
    // ════════════════════════════════════════════════════════════════

    struct GimbalConfig
    {
        // Identifiers
        ID id;
        Name name;

        // Internal config
        bool enable_roll;
        Link roll;
        bool enable_pitch;
        Link pitch;
        bool enable_yaw;
        Link yaw;
        float weight;
        float idle_power;
        float active_power;
    };

    struct CameraConfig
    {
        // Identifiers
        ID id;
        Name name;

        // Internal config
        CameraType type;
        Vector2i resolution;
        Vector2r sensor_size;
        Distortion distortion;
        bool enable_sensor_noise;
        SensorNoise sensor_noise;
        float weight;
        float idle_power;
        float active_power;
    };

    struct DroneConfig
    {
        // Identifiers
        ID id;
        Name name;

        // Internal config
        DroneType type;
        float cruise_speed;
        float max_speed;
        bool enable_barometer;
        Barometer barometer;
        bool enable_imu;
        Imu imu;
        bool enable_gps;
        Gps gps;
        bool enable_magnetometer;
        Magnetometer magnetometer;
        float base_weight;
        float max_payload_weight;
        float hover_power;
        float cruise_power;
        float load_factor;
    };

    // ════════════════════════════════════════════════════════════════
    // AGENT TYPES: Agent-related configuration types
    // ════════════════════════════════════════════════════════════════

    struct MultiWindowConfig
    {
        // Identifiers
        ID id;
        Name name;
        ID observation_set_id;

        // Internal config
        Vector2i resolution;
        float min_lambda;
        float max_lambda;
        float ref_lambda;
    };
    using MultiWindowConfigPtr = std::shared_ptr<MultiWindowConfig>;
    using MultiWindowSetConfig = std::map<ID, MultiWindowConfigPtr>;

    struct MultiCameraConfig
    {
        // Identifiers
        ID id;
        Name name;
        ID observation_set_id;

        // Internal config
        ID camera_id;
        ID gimbal_id;
        ObservationRole role;
        Vector3r position;
        Vector3r orientation;
        float min_focal;
        float max_focal;
        float ref_focal;
        std::string source_stream_url;
        std::string hardware;
        std::string ip;
        int port;

        // External config
        CameraConfig camera;
        GimbalConfig gimbal;
    };
    using MultiCameraConfigPtr = std::shared_ptr<MultiCameraConfig>;
    using MultiCameraSetConfig = std::map<ID, MultiCameraConfigPtr>;

    struct TrackingConfig
    {
        // Identifiers
        ID id;
        Name name;

        // Internal config
        ID observation_set_id;
        float min_target_size;
        float max_target_size;
        float ref_target_size;

        // External config
        MultiCameraSetConfig multi_camera_set;
        MultiWindowSetConfig multi_window_set;
    };

    struct AgentConfig
    {
        // Identifiers
        ID id;
        Name name;
        ID agent_team_id;

        // Internal config
        ID tracking_id;
        ID drone_id;
        Vector3r position;
        Vector3r orientation;
        float safety_radius;
        float max_altitude;
        float battery_capacity;

        // External config
        DroneConfig drone;
        TrackingConfig tracking;
    };
    using AgentConfigPtr = std::shared_ptr<AgentConfig>;
    using AgentTeamConfig = std::map<ID, AgentConfigPtr>;

    // ════════════════════════════════════════════════════════════════
    // GENERAL TYPES: Mission-related configuration types
    // ════════════════════════════════════════════════════════════════

    struct TargetConfig
    {
        // Identifiers
        ID id;
        Name name;
        ID target_group_id;
        int target_index;

        // Internal config
        TargetType type;
        int count;
        Priority priority;
        std::string trajectory_folder;
    };
    using TargetConfigPtr = std::shared_ptr<TargetConfig>;
    using TargetGroupConfig = std::map<ID, TargetConfigPtr>;

    struct EnvironmentConfig
    {
        // Identifiers
        ID id;
        Name name;

        // Internal config
        Coordinates geopoint;
        Vector3r wind_vel;
    };

    struct MissionConfig
    {
        // Identifiers
        ID id;
        Name name;

        // Internal config
        ID environment_id;
        ID target_group_id;
        ID agent_team_id;
        Vector2r horizontal_constraint;
        Vector2r vertical_constraint;
        Autopilot autopilot;
        DateTime start_date;
        HourTime start_hour;

        // External config
        EnvironmentConfig environment;
        TargetGroupConfig target_group;
        AgentTeamConfig agent_team;
        SystemParameters system;
        TopicParameters topics;
        FrameParameters frames;
    };
    using MissionConfigPtr = std::shared_ptr<MissionConfig>;

} // namespace flychams::core