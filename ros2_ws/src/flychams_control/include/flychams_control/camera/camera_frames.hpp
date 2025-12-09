#pragma once

// Base module include
#include "flychams_core/base/base_module.hpp"

// Core include
#include "flychams_core/utils/geo_utils.hpp"
#include "flychams_core/utils/math_utils.hpp"

namespace flychams::control
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
        CameraFrames(const core::ID& agent_id, core::NodePtr node, core::ConfigTools::SharedPtr config_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, config_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
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
            // Observation setpoint data
            core::AgentObservationSetpointsMsg setpoints;
            bool has_setpoints;
            // Subscriber
            core::SubscriberPtr<core::AgentObservationSetpointsMsg> setpoints_sub;
            // Constructor
            Agent()
                : setpoints(), has_setpoints(false), setpoints_sub()
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
        void setpointsCallback(const core::AgentObservationSetpointsMsg::SharedPtr msg);

    private: // Frames creation
        void createCameraBodyFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr);
        void createCameraOpticalFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr);

    private: // Frames update
        void updateCameraBodyFrame(const core::ID camera_id, const core::PointMsg& position, const core::QuaternionMsg& orientation);
    };

} // namespace flychams::control

