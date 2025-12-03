#pragma once

// Communication include
#include "flychams_core/communication/mavros_communication.hpp"

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::control
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief State manager for UAV drones
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-26
     * ════════════════════════════════════════════════════════════════
     */
    class DroneState : public core::BaseModule
    {
    public: // Constructor/Destructor
        DroneState(const core::ID& agent_id, core::NodePtr node, core::ConfigTools::SharedPtr config_tools, core::FrameworkTools::SharedPtr framework_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, config_tools, framework_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<DroneState>;
        struct Agent
        {
            // Odometry data
            core::OdometryMsg odom_in_msg;
            bool has_odom_in;
            // Status data (In)
            mavros_msgs::msg::State status_in_msg;
            bool has_status_in;
            // Status data (Out)
            core::AgentStatus status_out;
            // Status message (Out)
            core::AgentStatusMsg status_out_msg;
            // Position message
            core::PointStampedMsg position_out_msg;
            // Subscriber
            core::SubscriberPtr<mavros_msgs::msg::State> status_in_sub;
            core::SubscriberPtr<core::OdometryMsg> odom_in_sub;
            // Publishers
            core::PublisherPtr<core::AgentStatusMsg> status_out_pub;
            core::PublisherPtr<core::PointStampedMsg> position_out_pub;
            // Constructor
            Agent()
                : odom_in_msg(), has_odom_in(false), status_in_msg(), has_status_in(false),
                status_out(), status_out_msg(), position_out_msg(), status_in_sub(), odom_in_sub(),
                status_out_pub(), position_out_pub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;
        // Flight parameters
        float takeoff_altitude_;
        float landing_altitude_;

    private: // Data
        // Agent
        Agent agent_;
        // Last update time
        core::Time last_update_time_;
        // Mavros communication
        core::MavrosCommunication::SharedPtr mavros_comm_;

    private: // Callbacks
        void statusInCallback(const mavros_msgs::msg::State::SharedPtr msg);
        void odomInCallback(const core::OdometryMsg::SharedPtr msg);

    private: // State management
        void update();

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::control