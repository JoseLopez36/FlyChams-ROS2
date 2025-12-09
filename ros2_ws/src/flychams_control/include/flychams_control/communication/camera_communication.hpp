#pragma once

// AirSim interfaces includes
#include <airsim_interfaces/msg/gimbal_angle_cmd.hpp>
#include <airsim_interfaces/msg/camera_fov_cmd.hpp>
#include <airsim_interfaces/msg/camera_orientation.hpp>

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/ros_utils.hpp"

namespace flychams::control
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Camera communication interface for managing camera commands
     * (gimbal/camera)
     *
     * @details
     * This class provides utilities for managing the communication
     * with the camera.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-01
     * ════════════════════════════════════════════════════════════════
     */
    class CameraCommunication
    {
    public: // Constructors/Destructors
        CameraCommunication(const core::ID& agent_id, core::NodePtr node);
        virtual ~CameraCommunication();
        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<CameraCommunication>;

    public: // Camera state methods
        core::SubscriberPtr<airsim_interfaces::msg::CameraOrientation> subscribeCameraOrientation(const std::function<void(const airsim_interfaces::msg::CameraOrientation::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());

    public: // Camera control methods
        void setGimbalOrientations(const core::IDs& camera_ids, const std::vector<core::QuaternionMsg>& quaternions);
        void setCameraFovs(const core::IDs& camera_ids, const std::vector<float>& fovs);

    private: // Parameters
        core::ID agent_id_;

    private: // Data
        // ROS components
        core::NodePtr node_;

        // Publishers
        core::PublisherPtr<airsim_interfaces::msg::GimbalAngleCmd> gimbal_angle_cmd_pub_;
        core::PublisherPtr<airsim_interfaces::msg::CameraFovCmd> camera_fov_cmd_pub_;
    };

} // namespace flychams::control