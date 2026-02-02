#include "flychams_agent/camera/camera_communication.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    CameraCommunication::CameraCommunication(const core::ID& agent_id, NodePtr node, SettingsTools::SharedPtr settings_tools)
        : node_(node), agent_id_(agent_id), settings_tools_(settings_tools)
    {
        // Initialize ROS components
        gimbal_angle_cmd_pub_ = node_->create_publisher<airsim_interfaces::msg::GimbalAngleCmd>("/airsim/" + agent_id + "/gimbals/cmd/orientation", 10);
        camera_fov_cmd_pub_ = node_->create_publisher<airsim_interfaces::msg::CameraFovCmd>("/airsim/" + agent_id + "/cameras/cmd/fov", 10);

        // Initialize hardware drivers
        auto tracking_config = settings_tools_->getTracking(agent_id_);
        for (const auto& [cam_id, cam_config_ptr] : tracking_config.multi_camera_set)
        {
            if (cam_config_ptr->hardware == "SIYI A8 Mini")
            {
                // Instantiate driver
                auto driver = std::make_shared<SiyiA8Mini>();

                // Get IP/Port from parameters
                std::string ip = cam_config_ptr->ip;
                int port = cam_config_ptr->port;

                // Connect
                if (driver->connect(ip, port))
                {
                    RCLCPP_INFO(node_->get_logger(), "CameraCommunication: Connected to SIYI A8 Mini camera %s at %s:%d", cam_id.c_str(), ip.c_str(), port);
                    hardware_drivers_[cam_id] = driver;
                }
                else
                {
                    RCLCPP_ERROR(node_->get_logger(), "CameraCommunication: Failed to connect to SIYI A8 Mini camera %s at %s:%d", cam_id.c_str(), ip.c_str(), port);
                }
            }
        }
    }

    CameraCommunication::~CameraCommunication()
    {
        shutdown();
    }

    void CameraCommunication::shutdown()
    {
        // Disconnect hardware drivers
        for (auto& [id, driver] : hardware_drivers_)
        {
            if (driver)
            {
                driver->disconnect();
            }
        }
        hardware_drivers_.clear();

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
        // Create message for AirSim
        airsim_interfaces::msg::GimbalAngleCmd msg;
        msg.camera_names = camera_ids;
        msg.orientations = quaternions;

        // Publish message
        gimbal_angle_cmd_pub_->publish(msg);

        // Forward to hardware drivers
        for (size_t i = 0; i < camera_ids.size(); i++)
        {
            const auto& id = camera_ids[i];
            if (hardware_drivers_.find(id) != hardware_drivers_.end())
            {
                // Convert quaternion to Euler (RPY)
                Quaternionr q;
                RosUtils::fromMsg(quaternions[i], q);
                Vector3r euler = MavrosUtils::quatToEuler(q);
                
                // Convert to degrees
                float pitch = MathUtils::radToDeg(euler.y());
                float yaw = MathUtils::radToDeg(euler.z());
                
                // Send to driver (absolute yaw/pitch)
                hardware_drivers_[id]->setAngles(yaw, pitch);
            }
        }
    }

    void CameraCommunication::setCameraFovs(const IDs& camera_ids, const std::vector<float>& fovs)
    {
        // Create message for AirSim
        airsim_interfaces::msg::CameraFovCmd msg;
        msg.camera_names = camera_ids;
        msg.fovs = fovs;

        // Publish message
        camera_fov_cmd_pub_->publish(msg);

        // Forward to hardware drivers
        for (size_t i = 0; i < camera_ids.size(); i++)
        {
            const auto& id = camera_ids[i];
            if (hardware_drivers_.find(id) != hardware_drivers_.end())
            {
                // For now, we skip hardware zoom command
            }
        }
    }

} // namespace flychams::agent
