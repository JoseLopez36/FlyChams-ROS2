#pragma once

// Utils include
#include "flychams_agent/mavros/mavros_communication.hpp"

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief State manager for UAV drones
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-26
     * ════════════════════════════════════════════════════════════════
     */
    class DroneState : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        DroneState(const common::ID& agent_id, common::BaseStatusNode::SharedPtr node)
            : BaseStatusModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<DroneState>;
        struct Agent
        {
            // State data
            mavros_msgs::msg::State state;
            bool has_state;
            // Odometry data
            common::OdometryMsg local_odom;
            bool has_local_odom;
            // Subscriber
            common::SubscriberPtr<mavros_msgs::msg::State> state_sub;
            common::SubscriberPtr<common::OdometryMsg> local_odom_sub;
            // Publishers
            common::PublisherPtr<common::AgentStatusMsg> status_pub;
            common::PublisherPtr<common::PointStampedMsg> local_position_pub;
            common::PublisherPtr<common::PointStampedMsg> global_position_pub;
            // Constructor
            Agent()
                : state(), has_state(false), local_odom(), has_local_odom(false),
                state_sub(), local_odom_sub(),
                status_pub(), local_position_pub(), global_position_pub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;
        // Flight parameters
        float takeoff_altitude_;
        float landing_altitude_;

    private: // Data
        // Agent
        Agent agent_;
        // Mavros communication
        MavrosCommunication::SharedPtr mavros_comm_;

    private: // Callbacks
        void stateCallback(const mavros_msgs::msg::State::SharedPtr msg);
        void localOdomCallback(const common::OdometryMsg::SharedPtr msg);
        void armAllCallback(const common::BoolMsg::SharedPtr msg);
        void returnHomeCallback(const common::BoolMsg::SharedPtr msg);

    private: // Status management
        void update();
        bool checkStatus();
        
    private: // Command handlers
        void armAgent(const bool arm);
        void returnHome();

    private: // Status update methods
        void updateLocalPosition(const common::OdometryMsg& local_odom);
        void updateGlobalPosition(const common::OdometryMsg& local_odom);

    private: // ROS components
        // Subscribers
        common::SubscriberPtr<common::BoolMsg> arm_all_sub_;
        common::SubscriberPtr<common::BoolMsg> return_home_sub_;
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::agent