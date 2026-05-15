#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::coordinator
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Fleet manager - aggregates agent statuses and manages
     * the mission state machine.
     *
     * @details
     * Subscribes to all agent statuses via discovery, publishes
     * FleetStatus at 1Hz, and manages the MissionStatus state machine:
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
    class FleetStatus : public core::BaseModule
    {
    public: // Constructor/Destructor
        FleetStatus(core::NodePtr node, core::SettingsTools::SharedPtr settings_tools,
                     core::TopicTools::SharedPtr topic_tools,
                     core::TransformTools::SharedPtr transform_tools,
                     core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<FleetStatus>;
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
        void transitionMission(core::MissionState new_state);
        core::FleetState computeFleetState() const;

    private: // Parameters
        float update_rate_;

    private: // Data
        // Agents
        std::unordered_map<core::ID, Agent> agents_;
        // Mission state
        core::MissionState mission_state_;
        float mission_time_;
        core::Time mission_start_time_;
        std::vector<core::ID> active_agents_;

    private: // ROS components
        // Publishers
        core::PublisherPtr<core::FleetStatusMsg> fleet_status_pub_;
        core::PublisherPtr<core::MissionStatusMsg> mission_status_pub_;
        // Mission command subscriber (foxglove / operator)
        core::SubscriberPtr<core::StringMsg> mission_cmd_sub_;
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::coordinator
