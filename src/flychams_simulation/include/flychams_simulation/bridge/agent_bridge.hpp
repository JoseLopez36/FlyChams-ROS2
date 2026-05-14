#pragma once

// AirSim interfaces includes
#include <airsim_interfaces/msg/gimbal_angle_cmd.hpp>
#include <airsim_interfaces/msg/camera_fov_cmd.hpp>

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Bridge for agent simulation control
     *
     * @details
     * This class is responsible for bridging agent observation setpoints
     * to AirSim wrapper commands (CameraFovCmd and GimbalAngleCmd).
     * It listens to observation_setpoints per agent and publishes
     * the corresponding camera FOV and gimbal angle commands.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class AgentSimulationBridge : public core::BaseModule
    {
    public: // Constructor/Destructor
        AgentSimulationBridge(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentSimulationBridge>;
        using CameraFovCmdMsg = airsim_interfaces::msg::CameraFovCmd;
        using GimbalAngleCmdMsg = airsim_interfaces::msg::GimbalAngleCmd;
        struct Agent
        {
            // Observation setpoints subscriber
            core::SubscriberPtr<core::ObservationSetpointsMsg> observation_setpoints_sub;
            // Camera FOV command publisher
            core::PublisherPtr<airsim_interfaces::msg::CameraFovCmd> camera_fov_cmd_pub;
            // Gimbal angle command publisher
            core::PublisherPtr<airsim_interfaces::msg::GimbalAngleCmd> gimbal_angle_cmd_pub;
            // Latest observation setpoints
            core::ObservationSetpointsMsg observation_setpoints;
            bool has_observation_setpoints;
            // Constructor
            Agent()
                : observation_setpoints_sub(), camera_fov_cmd_pub(), gimbal_angle_cmd_pub(), observation_setpoints(), has_observation_setpoints(false)
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

    private: // Bridge management
        void update();
        void publishCameraFovCmd();
        void publishGimbalAngleCmd();

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::simulation
