#pragma once

// GStreamer includes
#include <gst/gst.h>

// Standard includes
#include <thread>
#include <atomic>

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
        struct StreamInfo
        {
            // URL
            std::string url;
            // URL info
            std::string protocol;
            std::string host;
            int port;
            // Parameters
            int width;
            int height;
            int bitrate;
        };

    private: // Parameters
        core::ID agent_id_;
        // YOLO stream parameters
        int yolo_width_;
        int yolo_height_;
        int yolo_bitrate_;
        // Central stream parameters
        int central_width_;
        int central_height_;
        int central_bitrate_;
        // Tracking streams parameters
        int tracking_width_;
        int tracking_height_;
        int tracking_bitrate_;
        // Stream info (for pipeline)
        StreamInfo source_stream_info_;
        StreamInfo yolo_stream_info_;
        StreamInfo central_stream_info_;
        std::vector<StreamInfo> tracking_stream_infos_;
        // GPU type
        std::string gpu_type_;
        // Source stream parameters
        int source_width_;
        int source_height_;

    private: // Data
        GstElement* pipeline_ = nullptr;
        std::vector<GstElement*> croppers_;
        std::atomic<bool> running_ = false;
        std::thread stream_thread_;

    private: // Callbacks
        void guiSetpointsCallback(const core::AgentGuiSetpointsMsg::SharedPtr msg);

    private: // Stream configuration
        StreamInfo getSourceStreamInfo(const core::MultiCameraConfigPtr& camera_config);
        StreamInfo getYoloStreamInfo(const core::AgentConfigPtr& agent_config);
        StreamInfo getInterfaceStreamInfo(const core::MultiCameraConfigPtr& camera_config);
        StreamInfo getInterfaceStreamInfo(const core::MultiWindowConfigPtr& window_config);
        void parseUrl(const std::string& url, std::string& protocol, std::string& host, int& port);
        std::string detectGpuType();
        std::string createPipeline(const std::string& gpu_type);
        std::string createNvidiaPipeline();
        std::string createAmdPipeline();

    private: // Stream management
        void startStream(const std::string& pipeline_str);
        void stopStream();

    private: // ROS components
        // Subscriber
        core::SubscriberPtr<core::AgentGuiSetpointsMsg> gui_setpoints_sub_;
    };

} // namespace flychams::agent