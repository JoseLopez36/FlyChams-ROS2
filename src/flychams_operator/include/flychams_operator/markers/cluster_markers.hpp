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
     * @brief Per-agent cluster marker generator for Foxglove
     *
     * @details
     * Subscribes to one agent's clusters topic. On each call to
     * getEntities() it appends bounding-sphere and ring entities for
     * every sub-cluster reported by that agent, coloured with the
     * agent's palette colour derived from the mission settings idx.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-23
     * ════════════════════════════════════════════════════════════════
     */
    class ClusterMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        ClusterMarkers(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
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

    public: // Entity collection
        void getEntities(common::FoxSceneUpdateMsg& out) const;

    private: // Parameters
        common::ID agent_id_;
        int agent_idx_ = 0;
        float update_rate_;

    private: // State
        std::vector<ClusterData> clusters_;
        bool has_data_ = false;

    private: // Callbacks
        void clustersCallback(const common::AgentClustersMsg::SharedPtr msg);

    private: // ROS components
        common::SubscriberPtr<common::AgentClustersMsg> clusters_sub_;
    };

} // namespace flychams::operator_pkg