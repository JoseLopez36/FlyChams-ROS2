#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Agent analysis manager
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-28
     * ════════════════════════════════════════════════════════════════
     */
    class AgentAnalysis : public core::BaseModule
    {
    public: // Constructor/Destructor
        AgentAnalysis(const core::ID& agent_id, core::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentAnalysis>;
        struct Agent
        {
            // Assignment data
            std::vector<core::ID> unit_ids;
            std::vector<core::ID> cluster_ids;
            core::ID central_unit_id;
            bool has_assignment;
            // Subscribers
            core::SubscriberPtr<core::AgentAssignmentMsg> assignment_sub;
            // Publisher
            core::PublisherPtr<core::AgentClustersMsg> clusters_pub;
            // Constructor
            Agent()
                : unit_ids(), cluster_ids(), has_assignment(false), assignment_sub(), clusters_pub()
            {
            }
        };
        struct Cluster
        {
            // Geometric data
            core::PointMsg center;
            float radius;
            bool has_geometry;
            // Subscribers
            core::SubscriberPtr<core::ClusterGeometryMsg> geometry_sub;
            // Constructor
            Cluster()
                : center(), radius(), has_geometry(false), geometry_sub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;

    private: // Data
        // Agent
        Agent agent_;
        // Clusters
        std::unordered_map<core::ID, Cluster> clusters_;

    private: // Callbacks
        void assignmentCallback(const core::AgentAssignmentMsg::SharedPtr msg);
        void clusterGeometryCallback(const core::ID& cluster_id, const core::ClusterGeometryMsg::SharedPtr msg);

    private: // Analysis management
        void update();
        void updateClusterSubscriptions(const std::vector<core::ID>& new_cluster_ids);

    private: // Analysis methods
        std::pair<core::PointMsg, float> computeCentralCluster(const std::vector<core::PointMsg>& centers, const std::vector<float>& radii);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::agent