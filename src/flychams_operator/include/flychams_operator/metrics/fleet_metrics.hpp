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
     * Tracks fleet-wide metrics including total agent count,
     * assignment solve duration and cumulative assignment swap count.
     * Publishes FleetMetrics message.
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
        bool has_assignment_solve_duration_;
        int assignment_swap_count_;

    public: // Element management
        void addAgent();
        void removeAgent();

    private: // Callbacks
        void assignmentSolveDurationCallback(const common::Float32Msg::SharedPtr msg);
        void assignmentSwapCountCallback(const common::Int32Msg::SharedPtr msg);

    private: // Update
        void update();
        bool checkStatus();

    private: // ROS components
        common::PublisherPtr<common::FleetMetricsMsg> metrics_pub_;
        common::SubscriberPtr<common::Float32Msg> assignment_solve_duration_sub_;
        common::SubscriberPtr<common::Int32Msg> assignment_swap_count_sub_;
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg