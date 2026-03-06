#pragma once

// GStreamer includes
#include <gst/gst.h>

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Class to handle video streaming using GStreamer
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-03-06
     * ════════════════════════════════════════════════════════════════
     */
    class AgentStream : public core::BaseModule
    {
    public: // Constructor/Destructor
        AgentStream(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentStream>;

    private: // Parameters
        core::ID agent_id_;
        std::string host_;
        int source_port_;
        int yolo_port_;
        int interface_port_;
        std::string gpu_type_;

    private: // Data
        GstElement* pipeline_;
        std::map<int, GstElement*> crops_;
        bool running_;

    private: // Callbacks
        void controlCallback(const core::StringMsg::SharedPtr msg);

    private: // Stream configuration
        std::string detectGpuType();
        std::string createPipeline();
        std::string createNvidiaPipeline();
        std::string createAmdPipeline();

    private: // Stream management
        void startStream();
        void stopStream();

    private: // ROS components
        core::SubscriberPtr<core::StringMsg> control_sub_;
    };

} // namespace flychams::agent