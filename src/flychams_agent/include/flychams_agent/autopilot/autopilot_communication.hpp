#pragma once

// PX4 includes
#include <px4_msgs/msg/home_position.hpp>
#include <px4_msgs/msg/vehicle_status.hpp>
#include <px4_msgs/msg/vehicle_odometry.hpp>
#include <px4_msgs/msg/vehicle_command.hpp>
#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief PX4 uXRCE-DDS interface for handling communication
     *
     * @details
     * This class provides utilities for managing the communication
     * with PX4 via the Micro-XRCE-DDS Agent.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-01
     * ════════════════════════════════════════════════════════════════
     */
    class AutopilotCommunication : public common::BaseModule
    {
    public: // Constructors/Destructors
        AutopilotCommunication(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AutopilotCommunication>;

    public: // Vehicle state methods
        common::SubscriberPtr<px4_msgs::msg::HomePosition> subscribeHomePosition(const std::function<void(const px4_msgs::msg::HomePosition::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        common::SubscriberPtr<px4_msgs::msg::VehicleStatus> subscribeVehicleStatus(const std::function<void(const px4_msgs::msg::VehicleStatus::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        common::SubscriberPtr<px4_msgs::msg::VehicleOdometry> subscribeLocalOdometry(const std::function<void(const px4_msgs::msg::VehicleOdometry::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());

    public: // Vehicle control methods
        bool armDisarm(const bool& arm);
        bool takeoff(const float& z);
        bool land();
        bool setMode(const std::string& mode);
        bool enableOffboard(const bool& enable);
        void setLocalPosition(const float& x, const float& y, const float& z);

    private: // Helpers
        void publishVehicleCommand(uint32_t command, float param1 = 0.0f, float param2 = 0.0f,
                                   float param3 = 0.0f, float param4 = 0.0f,
                                   float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);

    private: // Parameters
        common::ID agent_id_;

    private: // Data
        // Publishers
        common::PublisherPtr<px4_msgs::msg::VehicleCommand> vehicle_command_pub_;
        common::PublisherPtr<px4_msgs::msg::OffboardControlMode> offboard_control_mode_pub_;
        common::PublisherPtr<px4_msgs::msg::TrajectorySetpoint> trajectory_setpoint_pub_;
    };

} // namespace flychams::agent
