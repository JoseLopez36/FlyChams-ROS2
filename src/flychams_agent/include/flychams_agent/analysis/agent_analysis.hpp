#pragma once

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

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
    class AgentAnalysis : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        AgentAnalysis(const common::ID& agent_id, common::BaseStatusNode::SharedPtr node)
            : BaseStatusModule(node), agent_id_(agent_id)
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
            std::vector<common::ID> unit_ids;
            std::vector<common::ID> cluster_ids;
            bool has_assignment;
            // Subscribers
            common::SubscriberPtr<common::AgentAssignmentMsg> assignment_sub;
            // Publisher
            common::PublisherPtr<common::AgentClustersMsg> clusters_pub;
            // Constructor
            Agent()
                : unit_ids(), cluster_ids(), has_assignment(false), assignment_sub(), clusters_pub()
            {
            }
        };
        struct Cluster
        {
            // Geometric data
            common::PointMsg center;
            float radius;
            bool has_geometry;
            // Subscribers
            common::SubscriberPtr<common::ClusterGeometryMsg> geometry_sub;
            // Constructor
            Cluster()
                : center(), radius(), has_geometry(false), geometry_sub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;

    private: // Data
        // Agent
        Agent agent_;
        // Clusters
        std::unordered_map<common::ID, Cluster> clusters_;

    private: // Callbacks
        void assignmentCallback(const common::AgentAssignmentMsg::SharedPtr msg);
        void clusterGeometryCallback(const common::ID& cluster_id, const common::ClusterGeometryMsg::SharedPtr msg);

    private: // Analysis management
        void update();
        bool checkStatus();
        void updateClusterSubscriptions(const std::vector<common::ID>& new_cluster_ids);

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::agent