#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

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
    class CameraFrames : public core::BaseModule
    {
    public: // Constructor/Destructor
        CameraFrames(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<CameraFrames>;
        struct Agent
        {
            // Subscriber for observation setpoints
            core::SubscriberPtr<core::ObservationSetpointsMsg> observation_setpoints_sub;
            // Data
            core::ObservationSetpointsMsg observation_setpoints;
            bool has_observation_setpoints;
            // Constructor
            Agent()
                : observation_setpoints_sub(), observation_setpoints(), has_observation_setpoints(false)
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;

    private: // Data
        // Agent
        Agent agent_;

    private: // Callbacks
        void observationSetpointsCallback(const core::ObservationSetpointsMsg::SharedPtr msg);

    private: // Frames creation
        void createCameraOpticalFrame(const core::ID camera_id);

    private: // Frames management
        void update();

    private: // Frames update methods
        void updateCameraBodyFrame(const core::ID camera_id, const core::PointMsg& position, const core::QuaternionMsg& orientation);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::agent

