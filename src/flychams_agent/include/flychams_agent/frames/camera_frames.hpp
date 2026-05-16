#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Frame manager for UAV cameras
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-09
     * ════════════════════════════════════════════════════════════════
     */
    class CameraFrames : public common::BaseModule
    {
    public: // Constructor/Destructor
        CameraFrames(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<CameraFrames>;
        struct Agent
        {
            // Subscriber for observation setpoints
            common::SubscriberPtr<common::ObservationSetpointsMsg> observation_setpoints_sub;
            // Data
            common::ObservationSetpointsMsg observation_setpoints;
            bool has_observation_setpoints;
            // Constructor
            Agent()
                : observation_setpoints_sub(), observation_setpoints(), has_observation_setpoints(false)
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;

    private: // Data
        // Agent
        Agent agent_;

    private: // Callbacks
        void observationSetpointsCallback(const common::ObservationSetpointsMsg::SharedPtr msg);

    private: // Frames creation
        void createCameraOpticalFrame(const common::ID camera_id);

    private: // Frames management
        void update();

    private: // Frames update methods
        void updateCameraBodyFrame(const common::ID camera_id, const common::PointMsg& position, const common::QuaternionMsg& orientation);

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::agent

