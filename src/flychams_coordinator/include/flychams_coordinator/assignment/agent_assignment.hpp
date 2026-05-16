#pragma once

// Utils include
#include "flychams_common/assignment/assignment_solver.hpp"

// Base module include
#include "flychams_common/base/base_discoverer_module.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

namespace flychams::coordinator
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Assignment manager for UAV agents
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-01-29
     * ════════════════════════════════════════════════════════════════
     */
    class AgentAssignment : public core::BaseDiscovererModule
    {
    public: // Constructor/Destructor
        AgentAssignment(core::BaseDiscovererNode::SharedPtr node)
            : BaseDiscovererModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentAssignment>;
        struct Agent
        {
            // Status data
            core::AgentStatus status;
            bool has_status;
            // Unit data
            std::vector<core::ID> tracking_unit_ids;
            // Position data
            core::PointMsg position;
            bool has_position;
            // Subscribers
            core::SubscriberPtr<core::AgentStatusMsg> status_sub;
            core::SubscriberPtr<core::PointStampedMsg> position_sub;
            // Publisher
            core::PublisherPtr<core::AgentAssignmentMsg> assignment_pub;
            // Position solver
            core::PositionSolver::SharedPtr position_solver;
            // Constructor
            Agent()
                : status(), has_status(false), position(), has_position(false), status_sub(), position_sub(), assignment_pub(), position_solver()
            {
            }
            // Destructor
            ~Agent()
            {
                if (position_solver)
                {
                    position_solver->destroy();
                }
            }
        };
        struct Cluster
        {
            // Geometric data
            core::PointMsg center;
            float radius;
            bool has_geometry;
            // Subscriber
            core::SubscriberPtr<core::ClusterGeometryMsg> geometry_sub;
            // Constructor
            Cluster()
                : center(), radius(), has_geometry(false), geometry_sub()
            {
            }
        };

    private: // Parameters
        float update_rate_;
        // Position solver parameters
        core::PositionSolver::SolverMode position_solver_mode_;
        core::PositionSolver::Parameters position_solver_params_;
        // Transform parameters
        std::string world_frame_;
        std::unordered_map<core::ID, std::string> central_optical_frame_map_;

    private: // Data
        // Agents
        std::unordered_map<core::ID, Agent> agents_;
        std::set<core::ID> A_;
        // Clusters
        std::unordered_map<core::ID, Cluster> clusters_;
        std::set<core::ID> T_;
        // Assignment data
        core::RowVectorXi X_prev_;
        // Assignment solver
        core::AssignmentSolver::SharedPtr solver_;

    public: // Public methods
        void addAgent(const core::ID& agent_id);
        void addCluster(const core::ID& cluster_id);
        void removeAgent(const core::ID& agent_id);
        void removeCluster(const core::ID& cluster_id);

    private: // Callbacks
        void clusterGeometryCallback(const core::ID& cluster_id, const core::ClusterGeometryMsg::SharedPtr msg);
        void agentStatusCallback(const core::ID& agent_id, const core::AgentStatusMsg::SharedPtr msg);
        void agentPositionCallback(const core::ID& agent_id, const core::PointStampedMsg::SharedPtr msg);

    private: // Assignment management
        void update();

    private: // Utility methods
        core::PositionSolver::SharedPtr createPositionSolver(const std::string& agent_id, const core::PositionSolver::Parameters& solver_params, const core::PositionSolver::SolverMode& solver_mode);
        std::vector<core::CostFunctions::UnitCostParameters> createUnitParameters(const core::TrackingParameters& tracking_params);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::coordinator