#pragma once

// MavROS includes
#include <mavros_msgs/msg/home_position.hpp>
#include <mavros_msgs/msg/state.hpp>
#include <mavros_msgs/srv/command_bool.hpp>
#include <mavros_msgs/srv/command_tol.hpp>
#include <mavros_msgs/srv/set_mode.hpp>

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief MavROS interface for handling communication
     *
     * @details
     * This class provides utilities for managing the communication
     * with MavROS.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-01
     * ════════════════════════════════════════════════════════════════
     */
    class MavrosCommunication : public core::BaseModule
    {
    public: // Constructors/Destructors
        MavrosCommunication(const core::ID& agent_id, core::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<MavrosCommunication>;

    public: // Vehicle state methods
        core::SubscriberPtr<mavros_msgs::msg::HomePosition> subscribeHomePosition(const std::function<void(const mavros_msgs::msg::HomePosition::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        core::SubscriberPtr<mavros_msgs::msg::State> subscribeState(const std::function<void(const mavros_msgs::msg::State::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        core::SubscriberPtr<core::OdometryMsg> subscribeLocalOdometry(const std::function<void(const core::OdometryMsg::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());

    public: // Vehicle control methods
        bool armDisarm(const bool& arm);
        bool takeoff(const float& z);
        bool land();
        bool enableOffboard(const bool& enable);
        void setLocalPosition(const float& x, const float& y, const float& z);

    private: // Parameters
        core::ID agent_id_;

    private: // Data
        // Service clients
        core::ClientPtr<mavros_msgs::srv::CommandBool> arming_client_;
        core::ClientPtr<mavros_msgs::srv::CommandTOL> takeoff_client_;
        core::ClientPtr<mavros_msgs::srv::CommandTOL> land_client_;
        core::ClientPtr<mavros_msgs::srv::SetMode> set_mode_client_;

        // Publishers
        core::PublisherPtr<core::PoseStampedMsg> local_pos_pub_;
    };

} // namespace flychams::agent