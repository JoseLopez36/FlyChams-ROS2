#pragma once

// Utils include
#include "flychams_operator/markers/marker_parameters.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-cluster marker publisher for Foxglove visualization
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-18
     * ════════════════════════════════════════════════════════════════
     */
    class ClusterMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        ClusterMarkers(const common::ID& cluster_id, const std::unordered_map<ID, int>& agent_index_map, const common::ID& element_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), cluster_id_(cluster_id), agent_index_map_(agent_index_map), element_id_(element_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<ClusterMarkers>;
        struct ClusterData
        {
            common::PointMsg center;
            float radius  = 0.0f;
            common::ID unit_id;
        };
        struct AgentClusterData
        {
            std::vector<ClusterData> clusters;
            int  agent_idx = 0;
            bool has_data  = false;
        };

    private: // Parameters
        common::ID cluster_id_;
        std::unordered_map<ID, int> agent_index_map_;
        common::ID element_id_;
        float update_rate_;

    private: // Data
        // Per-agent cluster data, keyed by agent_id
        std::unordered_map<common::ID, AgentClusterData> agent_clusters_;

    private: // Callbacks
        void clustersCallback(const common::ID& agent_id, const common::AgentClustersMsg::SharedPtr msg);

    private: // Update
        void update();
        bool isDataValid() const;
        void buildClusterEntity(const common::ID& agent_id, size_t entry_idx, const AgentClusterData& data,
                                const std::string& frame, int64_t stamp_ns,
                                const rclcpp::Duration& lifetime,
                                common::FoxSceneUpdateMsg& out) const;

    private: // ROS components
        common::TimerPtr update_timer_;
        common::PublisherPtr<common::FoxSceneUpdateMsg> scene_pub_;
        std::vector<common::SubscriberPtr<common::AgentClustersMsg>> clusters_subs_;
    };

} // namespace flychams::operator_pkg