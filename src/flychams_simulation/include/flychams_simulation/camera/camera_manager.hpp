#pragma once

// AirSim interfaces include
#include "airsim_interfaces/srv/camera_capture.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Controller for agent cameras activation in the simulation
     *
     * @details
     * This class is responsible for managing the activation state of
     * agent cameras through the AirSim CameraCapture service. It handles
     * camera activation/deactivation requests for all agents.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-13
     * ════════════════════════════════════════════════════════════════
     */
    class CameraControl : public core::BaseModule
    {
    public: // Constructor/Destructor
        CameraControl(core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<CameraControl>;
        struct AgentCameraState
        {
            // Activation state
            bool active;
            // Service client
            rclcpp::Client<airsim_interfaces::srv::CameraCapture>::SharedPtr client;
            // Pending request flag
            bool request_pending;
            // Constructor
            AgentCameraState()
                : active(false), client(), request_pending(false)
            {
            }
        };

    private: // Data
        // Agent camera states
        std::unordered_map<core::ID, AgentCameraState> agent_cameras_;
        // Service name
        std::string camera_capture_service_name_;
        // Retry timer
        core::TimerPtr retry_timer_;
        // Retry rate
        float retry_rate_;

    public: // Public methods
        void addAgent(const core::ID& agent_id);
        void removeAgent(const core::ID& agent_id);
        void setAgentCameraActive(const core::ID& agent_id, bool active);

    private: // Control management
        void retryPendingRequests();

    private: // Service methods
        void callCameraCaptureService(const core::ID& agent_id, bool active);
        void handleServiceResponse(const core::ID& agent_id, bool active, rclcpp::Client<airsim_interfaces::srv::CameraCapture>::SharedFuture future);
    };

} // namespace flychams::simulation