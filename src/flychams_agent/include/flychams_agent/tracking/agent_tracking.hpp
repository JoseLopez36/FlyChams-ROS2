#pragma once

// Utils include
#include "flychams_common/tracking/observation_solver.hpp"

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Manager to ensure constant tracking of targets
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-01-29
     * ════════════════════════════════════════════════════════════════
     */
    class AgentTracking : public core::BaseStatusModule
    {
    public: // Constructor/Destructor
        AgentTracking(const core::ID& agent_id, core::BaseStatusNode::SharedPtr node)
            : BaseStatusModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentTracking>;
        struct Agent
        {
            // Status data
            core::AgentStatus status;
            bool has_status;
            // Clusters data
            core::AgentClustersMsg clusters;
            bool has_clusters;
            // Observation setpoint message
            core::ObservationSetpointsMsg observation_setpoints;
            // Subscribers
            core::SubscriberPtr<core::AgentStatusMsg> status_sub;
            core::SubscriberPtr<core::AgentClustersMsg> clusters_sub;
            // Publisher
            core::PublisherPtr<core::ObservationSetpointsMsg> observation_setpoints_pub;
            // Constructor
            Agent()
                : status(), has_status(false), clusters(), has_clusters(false), observation_setpoints(),
                status_sub(), clusters_sub(), observation_setpoints_pub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;
        // Tracking parameters
        core::TrackingParameters tracking_params_;
        // Transform parameters
        std::string world_frame_;
        std::vector<std::string> optical_frames_;
        int n_frames_;

    private: // Data
        // Agent
        Agent agent_;
        // Solvers
        std::vector<core::ObservationSolver::SharedPtr> solvers_;

    private: // Callbacks
        void statusCallback(const core::AgentStatusMsg::SharedPtr msg);
        void positionCallback(const core::PointStampedMsg::SharedPtr msg);
        void clustersCallback(const core::AgentClustersMsg::SharedPtr msg);

    private: // Tracking management
        void update();

    private: // Tracking methods
        std::tuple<float, core::Vector3r> updateCamera(const core::Vector3r& P, const float& r, const core::Matrix4r& T, core::ObservationSolver::SharedPtr solver);
        std::tuple<float, core::Crop> updateWindow(const core::Vector3r& P, const float& r, const core::Matrix4r& T, core::ObservationSolver::SharedPtr solver);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::agent