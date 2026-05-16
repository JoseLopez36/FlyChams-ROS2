#pragma once

// Utils include
#include "flychams_agent/mavros/mavros_communication.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

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
    class DroneState : public common::BaseModule
    {
    public: // Constructor/Destructor
        DroneState(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
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
            // Subscriber
            common::SubscriberPtr<mavros_msgs::msg::State> state_sub;
            common::SubscriberPtr<common::OdometryMsg> local_odom_sub;
            // Publishers
            common::PublisherPtr<common::AgentStatusMsg> status_pub;
            common::PublisherPtr<common::PointStampedMsg> local_position_pub;
            common::PublisherPtr<common::PointStampedMsg> global_position_pub;
            // Constructor
            Agent()
                : state(), has_state(false), state_sub(), local_odom_sub(),
                status_pub(), local_position_pub(), global_position_pub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
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

    private: // Status management
        void updateStatus(const mavros_msgs::msg::State& state, const common::OdometryMsg& local_odom);
        void updateLocalPosition(const common::OdometryMsg& local_odom);
        void updateGlobalPosition(const common::OdometryMsg& local_odom);
    };

} // namespace flychams::agent