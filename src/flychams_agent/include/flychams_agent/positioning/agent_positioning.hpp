#pragma once

// Position solver include
#include "flychams_common/positioning/position_solver.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

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
    class AgentPositioning : public core::BaseModule
    {
    public: // Constructor/Destructor
        AgentPositioning(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentPositioning>;
        struct Agent
        {
            // Status data
            core::AgentStatus status;
            bool has_status;
            // Position data
            core::PointMsg position;
            bool has_position;
            // Clusters data
            core::AgentClustersMsg clusters;
            bool has_clusters;
            // Setpoint message
            core::PointStampedMsg setpoint;
            // Subscribers
            core::SubscriberPtr<core::AgentStatusMsg> status_sub;
            core::SubscriberPtr<core::PointStampedMsg> position_sub;
            core::SubscriberPtr<core::AgentClustersMsg> clusters_sub;
            // Publisher
            core::PublisherPtr<core::PointStampedMsg> setpoint_pub;
            core::PublisherPtr<core::Float32Msg> optimization_duration_pub;
            // Constructor
            Agent()
                : status(), has_status(false), position(), has_position(false), clusters(),
                has_clusters(false), setpoint(), status_sub(), position_sub(), clusters_sub(),
                setpoint_pub(), optimization_duration_pub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;
        // Position solver parameters
        core::PositionSolver::SolverMode solver_mode_;
        core::PositionSolver::Parameters solver_params_;
        // Transform parameters
        std::string world_frame_;
        std::string central_optical_frame_;

    private: // Data
        // Agent
        Agent agent_;
        // Position solver
        core::PositionSolver::SharedPtr solver_;

    private: // Callbacks
        void statusCallback(const core::AgentStatusMsg::SharedPtr msg);
        void positionCallback(const core::PointStampedMsg::SharedPtr msg);
        void clustersCallback(const core::AgentClustersMsg::SharedPtr msg);

    private: // Positioning management
        void update();

    private: // Positioning methods
        core::PositionSolver::SharedPtr createSolver(const std::string& agent_id, const core::PositionSolver::Parameters& solver_params, const core::PositionSolver::SolverMode& solver_mode);
        std::vector<core::CostFunctions::UnitCostParameters> createUnitParameters(const core::TrackingParameters& tracking_params);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::agent