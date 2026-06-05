#pragma once

// Standard library includes
#include <chrono>
#include <future>

// Utils include
#include "flychams_common/assignment/assignment_solver.hpp"

// Base module include
#include "flychams_common/base/base_status_discoverer_module.hpp"

// Base node include
#include "flychams_common/base/base_status_discoverer_node.hpp"

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
    class AgentAssignment : public common::BaseStatusDiscovererModule
    {
    public: // Constructor/Destructor
        AgentAssignment(common::BaseStatusDiscovererNode::SharedPtr node)
            : BaseStatusDiscovererModule(node)
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
            // Unit data
            std::vector<common::ID> tracking_unit_ids;
            // Position data
            common::PointMsg position;
            bool has_position;
            // Subscribers
            common::SubscriberPtr<common::PointStampedMsg> position_sub;
            // Publisher
            common::PublisherPtr<common::AgentAssignmentMsg> assignment_pub;
            // Position solver
            common::PositionSolver::SharedPtr position_solver;
            // Constructor
            Agent()
                : tracking_unit_ids(), position(), has_position(false), position_sub(), assignment_pub(), position_solver()
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
            common::PointMsg center;
            float radius;
            bool has_geometry;
            // Subscriber
            common::SubscriberPtr<common::ClusterGeometryMsg> geometry_sub;
            // Constructor
            Cluster()
                : center(), radius(), has_geometry(false), geometry_sub()
            {
            }
        };

    private: // Parameters
        float update_rate_;
        float min_assignment_rate_;
        // Position solver parameters
        common::PositionSolver::SolverMode position_solver_mode_;
        common::PositionSolver::Parameters position_solver_params_;
        // Transform parameters
        std::string world_frame_;
        std::unordered_map<common::ID, std::string> central_optical_frame_map_;

    private: // Data
        // Agents
        std::unordered_map<common::ID, Agent> agents_;
        std::set<common::ID> A_;
        // Clusters
        std::unordered_map<common::ID, Cluster> clusters_;
        std::set<common::ID> T_;
        // Assignment data
        common::RowVectorXi X_prev_;
        // Assignment solver
        common::AssignmentSolver::SharedPtr solver_;

    public: // Public methods
        void addAgent(const common::ID& agent_id);
        void addCluster(const common::ID& cluster_id);
        void removeAgent(const common::ID& agent_id);
        void removeCluster(const common::ID& cluster_id);

    private: // Callbacks
        void clusterGeometryCallback(const common::ID& cluster_id, const common::ClusterGeometryMsg::SharedPtr msg);
        void agentPositionCallback(const common::ID& agent_id, const common::PointStampedMsg::SharedPtr msg);

    private: // Assignment management
        void update();
        bool checkStatus();
        void publishResult(const common::RowVectorXi& X, float time_elapsed_ms, int node_count);

    private: // Utility methods
        common::PositionSolver::SharedPtr createPositionSolver(const std::string& agent_id, const common::PositionSolver::Parameters& solver_params, const common::PositionSolver::SolverMode& solver_mode);
        std::vector<common::CostFunctions::UnitCostParameters> createUnitParameters(const common::TrackingParameters& tracking_params);

    private: // Async solve state
        std::future<std::tuple<common::RowVectorXi, float, int>> async_future_;
        std::vector<common::ID> async_agent_order_;
        std::vector<common::ID> async_cluster_order_;
        std::vector<common::PositionSolver::SharedPtr> async_solvers_;
        common::Time last_solve_time_;

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
        // Publisher for assignment solve duration
        common::PublisherPtr<common::Float32Msg> solve_duration_pub_;
        // Publisher for assignment evaluated node count
        common::PublisherPtr<common::Int32Msg> node_count_pub_;
        // Publisher for assignment swap count per solve
        common::PublisherPtr<common::Int32Msg> swap_count_pub_;
    };

} // namespace flychams::coordinator