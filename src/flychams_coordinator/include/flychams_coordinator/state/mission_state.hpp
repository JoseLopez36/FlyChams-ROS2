#pragma once

// Base module include
#include "flychams_common/base/base_discoverer_module.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

namespace flychams::coordinator
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Mission manager - manages mission lifecycle and progress.
     *
     * @details
     * Subscribes to fleet and mission status, publishes detailed
     * MissionState at 1Hz, and manages mission phases:
     *   PLANNING → EXECUTION → COMPLETION/ABORT
     *
     * Tracks mission objectives, progress, and completion criteria.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-16
     * ════════════════════════════════════════════════════════════════
     */
    class MissionState : public common::BaseDiscovererModule
    {
    public: // Constructor/Destructor
        MissionState(common::BaseDiscovererNode::SharedPtr node)
            : BaseDiscovererModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<MissionState>;

    private: // Parameters
        float update_rate_;

    private: // Data
        // Agents
        std::unordered_set<common::ID> agents_;
        // Fleet state
        common::FleetStatus fleet_status_;
        bool has_fleet_status_;
        // Mission state
        common::MissionStatus mission_status_;
        bool mission_active_;
        std::chrono::steady_clock::time_point mission_start_time_;
        float mission_time_;
        bool fleet_ready_;

    public: // Dynamic element management
        void addAgent(const common::ID& agent_id);
        void removeAgent(const common::ID& agent_id);

    private: // Callbacks
        void fleetStatusCallback(const common::FleetStatusMsg::SharedPtr msg);
        void startMissionCallback(const common::BoolMsg::SharedPtr msg);
        void pauseMissionCallback(const common::BoolMsg::SharedPtr msg);
        void abortMissionCallback(const common::BoolMsg::SharedPtr msg);

    private: // Update loop
        void update();
        bool checkStatus();

    private: // ROS components
        // Subscribers
        common::SubscriberPtr<common::FleetStatusMsg> fleet_status_sub_;
        common::SubscriberPtr<common::BoolMsg> start_mission_sub_;
        common::SubscriberPtr<common::BoolMsg> pause_mission_sub_;
        common::SubscriberPtr<common::BoolMsg> abort_mission_sub_;
        // Publishers
        common::PublisherPtr<common::MissionStatusMsg> mission_status_pub_;
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::coordinator