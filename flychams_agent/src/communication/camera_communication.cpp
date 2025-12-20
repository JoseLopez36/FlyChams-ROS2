#include "flychams_agent/communication/camera_communication.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    CameraCommunication::CameraCommunication(const core::ID& agent_id, NodePtr node)
        : node_(node), agent_id_(agent_id)
    {
        // Initialize ROS components
        gimbal_angle_cmd_pub_ = node_->create_publisher<airsim_interfaces::msg::GimbalAngleCmd>("/airsim/" + agent_id + "/gimbals/cmd/orientation", 10);
        camera_fov_cmd_pub_ = node_->create_publisher<airsim_interfaces::msg::CameraFovCmd>("/airsim/" + agent_id + "/cameras/cmd/fov", 10);
    }

    CameraCommunication::~CameraCommunication()
    {
        shutdown();
    }

    void CameraCommunication::shutdown()
    {
        // Destroy publishers
        gimbal_angle_cmd_pub_.reset();
        camera_fov_cmd_pub_.reset();
        // Destroy node pointer
        node_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CAMERA STATE
    // ════════════════════════════════════════════════════════════════════════════

    core::SubscriberPtr<airsim_interfaces::msg::CameraOrientation> CameraCommunication::subscribeCameraOrientation(const std::function<void(const airsim_interfaces::msg::CameraOrientation::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options)
    {
        return node_->create_subscription<airsim_interfaces::msg::CameraOrientation>("/airsim/" + agent_id_ + "/cameras/state/orientation", 10, callback, options);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CAMERA CONTROL
    // ════════════════════════════════════════════════════════════════════════════

    void CameraCommunication::setGimbalOrientations(const IDs& camera_ids, const std::vector<QuaternionMsg>& quaternions)
    {
        // Create message
        airsim_interfaces::msg::GimbalAngleCmd msg;
        msg.camera_names = camera_ids;
        msg.orientations = quaternions;

        // Publish message
        gimbal_angle_cmd_pub_->publish(msg);
    }

    void CameraCommunication::setCameraFovs(const IDs& camera_ids, const std::vector<float>& fovs)
    {
        // Create message
        airsim_interfaces::msg::CameraFovCmd msg;
        msg.camera_names = camera_ids;
        msg.fovs = fovs;

        // Publish message
        camera_fov_cmd_pub_->publish(msg);
    }

} // namespace flychams::agent
