#pragma once

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_discoverer_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Mission-level metrics aggregator
     *
     * @details
     * Subscribes to fleet status and tracks mission-level metrics
     * including total counts of agents, targets, clusters, and
     * elapsed mission time. Publishes MissionMetrics message.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-24
     * ════════════════════════════════════════════════════════════════
     */
    class MissionMetrics : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        MissionMetrics(common::BaseStatusDiscovererNode::SharedPtr node)
            : BaseStatusModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<MissionMetrics>;

    private: // Parameters
        float update_rate_;

    private: // Accumulated data
        int total_agents_;
        int total_targets_;
        int total_clusters_;
        common::Time mission_start_time_;
        bool has_mission_started_;

    public: // Element management
        void addAgent();
        void removeAgent();
        void addTarget();
        void removeTarget();
        void addCluster();
        void removeCluster();

    private: // Update
        void update();
        bool checkStatus();

    private: // ROS components
        common::PublisherPtr<common::MissionMetricsMsg> metrics_pub_;
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg