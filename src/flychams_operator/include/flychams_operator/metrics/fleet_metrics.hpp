#pragma once

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_discoverer_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Fleet-level metrics aggregator
     *
     * @details
     * Tracks fleet-wide metrics including total agent count and
     * assignment solve duration. Publishes FleetMetrics message.
     *
     * Note: assignment_solve_duration is currently not sourced from
     * the coordinator (placeholder value).
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-24
     * ════════════════════════════════════════════════════════════════
     */
    class FleetMetrics : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        FleetMetrics(common::BaseStatusDiscovererNode::SharedPtr node)
            : BaseStatusModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<FleetMetrics>;

    private: // Parameters
        float update_rate_;

    private: // Accumulated data
        int total_agents_;
        float assignment_solve_duration_;

    public: // Element management
        void addAgent();
        void removeAgent();

    private: // Update
        void update();
        bool checkStatus();

    private: // ROS components
        common::PublisherPtr<common::FleetMetricsMsg> metrics_pub_;
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg