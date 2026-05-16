#pragma once

// Base module include
#include "flychams_common/base/base_discoverer_module.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

namespace flychams::coordinator
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Fleet manager - aggregates agent statuses and manages
     * the mission state machine.
     *
     * @details
     * Subscribes to all agent statuses via discovery, publishes
     * FleetState at 1Hz, and manages the MissionStatus state machine:
     *   READY → ACTIVE → PAUSED → ACTIVE (resume)
     *   Any  → ABORTED (emergency)
     *
     * Mission command topic receives std_msgs/String with values:
     *   "start", "pause", "resume", "abort"
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class FleetState : public common::BaseDiscovererModule
    {
    public: // Constructor/Destructor
        FleetState(common::BaseDiscovererNode::SharedPtr node)
            : BaseDiscovererModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<FleetState>;
        struct Agent
        {
            // Status data
            common::AgentStatus status;
            bool has_status;
            // Subscriber
            common::SubscriberPtr<common::AgentStatusMsg> status_sub;
            // Constructor
            Agent()
                : status(common::AgentStatus::IDLE), has_status(false), status_sub()
            {
            }
        };

    public: // Dynamic element management
        void addAgent(const common::ID& agent_id);
        void removeAgent(const common::ID& agent_id);

    private: // Callbacks
        void agentStatusCallback(const common::ID& agent_id, const common::AgentStatusMsg::SharedPtr msg);
        void missionCmdCallback(const common::StringMsg::SharedPtr msg);

    private: // Update loop
        void update();

    private: // State machine helpers
        void transitionMission(common::MissionStatus new_status);
        common::FleetStatus computeFleetStatus() const;

    private: // Parameters
        float update_rate_;

    private: // Data
        // Agents
        std::unordered_map<common::ID, Agent> agents_;
        // Mission state
        common::MissionStatus mission_status_;
        float mission_time_;
        common::Time mission_start_time_;
        std::vector<common::ID> active_agents_;

    private: // ROS components
        // Subscribers
        common::SubscriberPtr<common::StringMsg> mission_cmd_sub_;
        // Publishers
        common::PublisherPtr<common::FleetStatusMsg> fleet_status_pub_;
        common::PublisherPtr<common::MissionStatusMsg> mission_status_pub_;
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::coordinator
