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
    class FleetState : public core::BaseDiscovererModule
    {
    public: // Constructor/Destructor
        FleetState(core::BaseDiscovererNode::SharedPtr node)
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
            core::AgentStatus status;
            bool has_status;
            // Subscriber
            core::SubscriberPtr<core::AgentStatusMsg> status_sub;
            // Constructor
            Agent()
                : status(core::AgentStatus::IDLE), has_status(false), status_sub()
            {
            }
        };

    public: // Dynamic element management
        void addAgent(const core::ID& agent_id);
        void removeAgent(const core::ID& agent_id);

    private: // Callbacks
        void agentStatusCallback(const core::ID& agent_id, const core::AgentStatusMsg::SharedPtr msg);
        void missionCmdCallback(const core::StringMsg::SharedPtr msg);

    private: // Update loop
        void update();

    private: // State machine helpers
        void transitionMission(core::MissionStatus new_status);
        core::FleetStatus computeFleetStatus() const;

    private: // Parameters
        float update_rate_;

    private: // Data
        // Agents
        std::unordered_map<core::ID, Agent> agents_;
        // Mission state
        core::MissionStatus mission_status_;
        float mission_time_;
        core::Time mission_start_time_;
        std::vector<core::ID> active_agents_;

    private: // ROS components
        // Subscribers
        core::SubscriberPtr<core::StringMsg> mission_cmd_sub_;
        // Publishers
        core::PublisherPtr<core::FleetStatusMsg> fleet_status_pub_;
        core::PublisherPtr<core::MissionStatusMsg> mission_status_pub_;
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::coordinator
