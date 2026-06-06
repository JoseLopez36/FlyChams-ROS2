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
    class AgentTracking : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        AgentTracking(const common::ID& agent_id, common::BaseStatusNode::SharedPtr node)
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
            common::AgentStatus status;
            bool has_status;
            // Clusters data
            common::AgentClustersMsg clusters;
            bool has_clusters;
            // Observation setpoint message
            common::ObservationSetpointsMsg observation_setpoints;
            // Subscribers
            common::SubscriberPtr<common::AgentStatusMsg> status_sub;
            common::SubscriberPtr<common::AgentClustersMsg> clusters_sub;
            // Publisher
            common::PublisherPtr<common::ObservationSetpointsMsg> observation_setpoints_pub;
            // Constructor
            Agent()
                : status(), has_status(false), clusters(), has_clusters(false), observation_setpoints(),
                status_sub(), clusters_sub(), observation_setpoints_pub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;
        // Tracking parameters
        common::TrackingParameters tracking_params_;
        // Transform parameters
        std::string world_frame_;
        std::vector<std::string> optical_frames_;
        int n_frames_;

    private: // Data
        // Agent
        Agent agent_;
        // Solvers
        std::vector<common::ObservationSolver::SharedPtr> solvers_;

    private: // Callbacks
        void statusCallback(const common::AgentStatusMsg::SharedPtr msg);
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);
        void clustersCallback(const common::AgentClustersMsg::SharedPtr msg);

    private: // Tracking management
        void update();
        bool checkStatus();

    private: // Tracking methods
        std::tuple<float, float, common::Vector3r, float> updateCamera(const common::Vector3r& P, const float& r, const common::Matrix4r& T, common::ObservationSolver::SharedPtr solver);
        std::tuple<float, float, float, common::Crop, float> updateWindow(const common::Vector3r& P, const float& r, const common::Matrix4r& T, common::ObservationSolver::SharedPtr solver);

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::agent