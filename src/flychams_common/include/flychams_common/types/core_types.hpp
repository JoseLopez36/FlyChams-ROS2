#pragma once

// Standard includes
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>

// Eigen includes
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Types used throughout the project
     *
     * @details
     * This file contains all the types used throughout the project,
     * including basic types, geometry types, and other types.
     * ════════════════════════════════════════════════════════════════
    */

    // ════════════════════════════════════════════════════════════════
    // MATHEMATICAL TYPES: Eigen types used throughout the project
    // ════════════════════════════════════════════════════════════════

    using Vector2r = Eigen::Vector2f;
    using Vector2i = Eigen::Vector2i;
    using Vector2d = Eigen::Vector2d;

    using Vector3r = Eigen::Vector3f;
    using Vector3i = Eigen::Vector3i;
    using Vector3d = Eigen::Vector3d;

    using Vector4r = Eigen::Vector4f;
    using Vector4i = Eigen::Vector4i;
    using Vector4d = Eigen::Vector4d;

    using VectorXr = Eigen::VectorXf;
    using VectorXi = Eigen::VectorXi;
    using VectorXd = Eigen::VectorXd;

    using RowVector2r = Eigen::RowVector2f;
    using RowVector2i = Eigen::RowVector2i;
    using RowVector2d = Eigen::RowVector2d;

    using RowVector3r = Eigen::RowVector3f;
    using RowVector3i = Eigen::RowVector3i;
    using RowVector3d = Eigen::RowVector3d;

    using RowVector4r = Eigen::RowVector4f;
    using RowVector4i = Eigen::RowVector4i;
    using RowVector4d = Eigen::RowVector4d;

    using RowVectorXr = Eigen::RowVectorXf;
    using RowVectorXi = Eigen::RowVectorXi;
    using RowVectorXd = Eigen::RowVectorXd;

    using Matrix2r = Eigen::Matrix2f;
    using Matrix2i = Eigen::Matrix2i;
    using Matrix2d = Eigen::Matrix2d;

    using Matrix3r = Eigen::Matrix3f;
    using Matrix3i = Eigen::Matrix3i;
    using Matrix3d = Eigen::Matrix3d;

    using Matrix4r = Eigen::Matrix4f;
    using Matrix4i = Eigen::Matrix4i;
    using Matrix4d = Eigen::Matrix4d;

    using MatrixXr = Eigen::MatrixXf;
    using MatrixXi = Eigen::MatrixXi;
    using MatrixXd = Eigen::MatrixXd;

    using Matrix2Xr = Eigen::Matrix2Xf;
    using Matrix2Xi = Eigen::Matrix2Xi;
    using Matrix2Xd = Eigen::Matrix2Xd;

    using Matrix3Xr = Eigen::Matrix3Xf;
    using Matrix3Xi = Eigen::Matrix3Xi;
    using Matrix3Xd = Eigen::Matrix3Xd;

    using Matrix4Xr = Eigen::Matrix4Xf;
    using Matrix4Xi = Eigen::Matrix4Xi;
    using Matrix4Xd = Eigen::Matrix4Xd;

    using Quaternionr = Eigen::Quaternionf;

    // ════════════════════════════════════════════════════════════════
    // IDENTIFIER TYPES: Identifier types used throughout the project
    // ════════════════════════════════════════════════════════════════

    using ID = std::string;
    using IDs = std::vector<ID>;
    using Name = std::string;

    // ════════════════════════════════════════════════════════════════
    // ENUM TYPES: Enum types used throughout the project
    // ════════════════════════════════════════════════════════════════

    /**
     * Enum for element types
     */
    enum class ElementType
    {
        None,    // No type assigned
        Agent,   // Aerial vehicle (UAV)
        Target,  // Target to track
        Cluster  // Group of targets
    };

    /**
     * @brief Simulation framework enumeration
     */
    enum class SimulationFramework
    {
        None,    // No framework assigned
        AirSim,  // AirSim framework
        Gazebo,  // Gazebo framework
        IsaacSim // IsaacSim framework
    };
    inline SimulationFramework simulationFrameworkFromString(const std::string& framework)
    {
        if (framework == "AirSim") return SimulationFramework::AirSim;
        if (framework == "Gazebo") return SimulationFramework::Gazebo;
        if (framework == "IsaacSim") return SimulationFramework::IsaacSim;
        return SimulationFramework::None;
    }

    /**
     * Enum for autopilot types
     */
    enum class Autopilot
    {
        None,           // No type assigned
        SimpleFlight,   // Simple flight autopilot (AirSim's default)
        PX4             // PX4 autopilot
    };
    inline Autopilot autopilotFromString(const std::string& autopilot_type)
    {
        if (autopilot_type == "SimpleFlight") return Autopilot::SimpleFlight;
        if (autopilot_type == "PX4") return Autopilot::PX4;
        return Autopilot::None;
    }

    /**
     * Enum for tracking modes
     */
    enum class TrackingMode
    {
        None,                   // No tracking mode assigned
        MultiCamera,            // Multiple orientable and zoom-adjustable cameras tracking targets
        MultiWindow,            // Multiple tracking windows in a single ultra-high-resolution camera
        MultiHybrid             // Hybrid tracking based on multiple cameras and windows
    };
    inline std::string trackingModeToString(const TrackingMode& tracking_mode)
    {
        if (tracking_mode == TrackingMode::MultiCamera) return "MultiCamera";
        if (tracking_mode == TrackingMode::MultiWindow) return "MultiWindow";
        if (tracking_mode == TrackingMode::MultiHybrid) return "MultiHybrid";
        return "None";
    }

    /**
     * Enum for observation unit types
     */
    enum class ObservationType
    {
        None,     // No type assigned
        Camera,   // Camera type
        Window  // Window type
    };
    inline std::string observationTypeToString(const ObservationType& observation_type)
    {
        if (observation_type == ObservationType::Camera) return "Camera";
        if (observation_type == ObservationType::Window) return "Window";
        return "None";
    }

    /**
     * Enum for observation unit roles
     */
    enum class ObservationRole
    {
        None,     // No role assigned
        Central,  // Central role
        Tracking  // Tracking role
    };
    inline ObservationRole observationRoleFromString(const std::string& observation_role)
    {
        if (observation_role == "Central") return ObservationRole::Central;
        if (observation_role == "Tracking") return ObservationRole::Tracking;
        return ObservationRole::None;
    }
    inline std::string observationRoleToString(const ObservationRole& observation_role)
    {
        if (observation_role == ObservationRole::Central) return "Central";
        if (observation_role == ObservationRole::Tracking) return "Tracking";
        return "None";
    }

    /**
     * Enum for target types
     */
    enum class TargetType
    {
        None,     // No type assigned
        Human,    // Human target
    };
    inline TargetType targetTypeFromString(const std::string& target_type)
    {
        if (target_type == "Human") return TargetType::Human;
        return TargetType::None;
    }

    /**
     * Enum for target priority
     */
    enum class Priority
    {
        None,     // No priority assigned
        Low,      // Low priority
        Medium,   // Medium priority
        High      // High priority
    };
    inline Priority priorityFromString(const std::string& priority)
    {
        if (priority == "Low") return Priority::Low;
        if (priority == "Medium") return Priority::Medium;
        if (priority == "High") return Priority::High;
        return Priority::None;
    }

    /**
     * Enum for drone types
     */
    enum class DroneType
    {
        None,           // No type assigned
        Quadcopter,     // Quadcopter
        Hexacopter      // Hexacopter
    };
    inline DroneType droneTypeFromString(const std::string& drone_type)
    {
        if (drone_type == "Quadcopter") return DroneType::Quadcopter;
        if (drone_type == "Hexacopter") return DroneType::Hexacopter;
        return DroneType::None;
    }

    /**
     * Enum for camera types
     */
    enum class CameraType
    {
        None,           // No type assigned
        RGB,            // Standard RGB camera
        Infrared,       // Infrared camera
        Depth           // Depth camera
    };
    inline CameraType cameraTypeFromString(const std::string& camera_type)
    {
        if (camera_type == "RGB") return CameraType::RGB;
        if (camera_type == "Infrared") return CameraType::Infrared;
        if (camera_type == "Depth") return CameraType::Depth;
        return CameraType::None;
    }

    /**
     * Enum for mission status
     */
    enum class MissionStatus
    {
        READY,    // 0: Fleet can accept mission start
        ACTIVE,   // 1: Mission is running
        PAUSED,   // 2: Mission paused, agents hovering
        ABORTED   // 3: Mission aborted, agents landing
    };

    /**
     * Enum for fleet status
     */
    enum class FleetStatus
    {
        IDLE,    // 0: All agents in IDLE
        ACTIVE,  // 1: All agents in ACTIVE
        MIXED,   // 2: Agents in mixed states
        ERROR    // 3: Any agent in ERROR
    };

    /**
     * Enum for agent status
     */
    enum class AgentStatus
    {
        IDLE,    // 0: On ground, disarmed, safe
        ACTIVE,  // 1: Armed, flying, executing mission
        ERROR    // 2: Fault condition, requires reset
    };

    // ════════════════════════════════════════════════════════════════
    // TRACKING TYPES: Tracking-related types used throughout the project
    // ════════════════════════════════════════════════════════════════

    /**
     * Camera parameters (for multi-camera units)
     */
    struct CameraParameters
    {
        // Camera resolution (pix)
        int width;
        int height;
        // Sensor dimensions (m)
        float sensor_width;
        float sensor_height;
        // Camera intrinsic matrix K
        Matrix3r K;
    };

    /**
     * Window parameters (for multi-window units)
     */
    struct WindowParameters
    {
        // Central camera focal length (m)
        float f_ref;
        // Full resolution (pix)
        int full_width;
        int full_height;
        // Tracking resolution (pix)
        int tracking_width;
        int tracking_height;
    };

    /**
     * Observation unit parameters (either multi-camera or multi-window units)
     */
    struct ObservationUnitParameters
    {
        // Unit ID
        std::string id;
        // Unit type
        ObservationType type;
        // Unit role
        ObservationRole role;
        // Zoom factor limits
        float upsilon_min;
        float upsilon_max;
        float upsilon_ref;
        // Regularized pixel size (m/pix)
        float rho_x;
        float rho_y;
        float rho;
        // Apparent target sizes (pix)
        float s_min_pix;
        float s_max_pix;
        float s_ref_pix;
        // Apparent target sizes (m)
        float s_min;
        float s_max;
        float s_ref;

        // Camera parameters (for Camera and Window (central camera parameters) types)
        CameraParameters camera_params;

        // Window parameters (for Window type)
        WindowParameters window_params;
    };

    /**
     * Tracking parameters
     */
    struct TrackingParameters
    {
        // Tracking mode
        TrackingMode mode;
        // Number of units
        int n_o;                                          // Number of observation units (n_t + 1)
        int n_t;                                          // Number of tracking units (n_c + n_w)
        int n_c;                                          // Number of tracking cameras
        int n_w;                                          // Number of tracking windows
        // Parameters for each unit (vector of n_o elements)
        std::vector<ObservationUnitParameters> observation_units_params;
    };

    // ════════════════════════════════════════════════════════════════
    // OTHER TYPES: Other types used throughout the project
    // ════════════════════════════════════════════════════════════════

    struct DateTime
    {
        uint32_t year;
        uint32_t month;
        uint32_t day;
    };

    struct HourTime
    {
        uint32_t hours;
        uint32_t minutes;
        uint32_t seconds;
    };

    struct Coordinates
    {
        double latitude;
        double longitude;
        double altitude;
    };

    struct Barometer
    {
        float white_noise_sigma;        // White noise sigma
    };

    struct Imu
    {
        float angular_white_noise_sigma;    // Angular white noise sigma
        float velocity_white_noise_sigma;   // Velocity white noise sigma
    };

    struct Gps
    {
        float eph_initial;       // Initial horizontal position accuracy
        float epv_initial;       // Initial vertical position accuracy
        float eph_final;         // Final horizontal position accuracy
        float epv_final;         // Final vertical position accuracy
    };

    struct Magnetometer
    {
        float white_noise_sigma;       // White noise sigma
        float white_noise_bias;        // White noise bias
    };

    struct Link
    {
        float min_angle;
        float max_angle;
        float max_speed;
    };

    struct Distortion
    {
        float K1;
        float K2;
        float K3;
        float P1;
        float P2;
    };

    struct SensorNoise
    {
        float rand_contrib;
        float rand_size;
        float rand_speed;
    };

    struct Crop
    {
        int x;
        int y;
        int w;
        int h;
        bool is_out_of_bounds;
    };

} // namespace flychams::core 