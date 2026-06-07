#pragma once

// Utils include
#include "flychams_common/positioning/position_solver.hpp"

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Agent positioning module
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-28
     * ════════════════════════════════════════════════════════════════
     */
    class AgentPositioning : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        AgentPositioning(const common::ID& agent_id, common::BaseStatusNode::SharedPtr node)
            : BaseStatusModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentPositioning>;
        struct Agent
        {
            // Status data
            common::AgentStatus status;
            bool has_status;
            // Position data
            common::PointMsg position;
            bool has_position;
            // Clusters data
            common::AgentClustersMsg clusters;
            bool has_clusters;
            // Setpoint message
            common::PointStampedMsg setpoint;
            // Subscribers
            common::SubscriberPtr<common::AgentStatusMsg> status_sub;
            common::SubscriberPtr<common::PointStampedMsg> position_sub;
            common::SubscriberPtr<common::AgentClustersMsg> clusters_sub;
            // Publishers
            common::PublisherPtr<common::PointStampedMsg> setpoint_pub;
            common::PublisherPtr<common::Float32Msg> solve_duration_pub;
            // Constructor
            Agent()
                : status(), has_status(false), position(), has_position(false), clusters(),
                has_clusters(false), setpoint(), status_sub(), position_sub(), clusters_sub(),
                setpoint_pub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;
        // Position solver parameters
        common::PositionSolver::SolverMode solver_mode_;
        common::PositionSolver::Parameters solver_params_;
        common::CostFunctions::UnitCostWeights cost_weights_;

    private: // Data
        // Agent
        Agent agent_;
        // Position solver
        common::PositionSolver::SharedPtr solver_;

    private: // Callbacks
        void statusCallback(const common::AgentStatusMsg::SharedPtr msg);
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);
        void clustersCallback(const common::AgentClustersMsg::SharedPtr msg);

    private: // Positioning management
        void update();
        bool checkStatus();

    private: // Positioning methods
        common::PositionSolver::SharedPtr createSolver(const std::string& agent_id, const common::PositionSolver::Parameters& solver_params, const common::PositionSolver::SolverMode& solver_mode);
        std::vector<common::CostFunctions::UnitCostParameters> createUnitParameters(const common::TrackingParameters& tracking_params);

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::agent