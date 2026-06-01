#pragma once

// Standard includes
#include <cstdio>
#include <fstream>
#include <sstream>

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
     * including total counts of agents, targets, clusters, elapsed
     * mission time, and system performance metrics (CPU, GPU, RAM,
     * VRAM). Publishes MissionMetrics message. GPU and VRAM sampling
     * supports both NVIDIA (nvidia-smi) and AMD (AMDGPU sysfs) vendors,
     * determined at init time from the HW_VENDOR environment variable.
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

        struct HwSample
        {
            float instant; // Last sampled value
            float sum;     // Accumulated sum (for mean)
            float max;     // Peak value
            int   count;   // Number of samples collected
        };

    private: // Parameters
        float update_rate_;
        bool enable_performance_metrics_;
        std::string hw_vendor_; // "nvidia" | "amd" | "none"

    private: // Accumulated data
        int total_agents_;
        int total_targets_;
        int total_clusters_;
        common::Time mission_start_time_;
        bool has_mission_started_;

        HwSample cpu_;
        HwSample gpu_;
        HwSample ram_;
        HwSample vram_;

    public: // Element management
        void addAgent();
        void removeAgent();
        void addTarget();
        void removeTarget();
        void addCluster();
        void removeCluster();

    private: // Sampling helpers
        float readCpuUsage() const;
        float readGpuUsage() const;
        float readRamUsageGb() const;
        float readVramUsageGb() const;
        void  sampleHardware();
        void  resetHwSamples();
        static void updateSample(HwSample& s, float value);

    private: // Update
        void update();
        bool checkStatus();

    private: // ROS components
        common::PublisherPtr<common::MissionMetricsMsg> metrics_pub_;
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg