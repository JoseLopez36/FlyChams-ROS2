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
        // Full stream parameters
        int full_width_;
        int full_height_;
        int full_bitrate_;
        // Crop streams parameters
        int crop_width_;
        int crop_height_;
        int crop_bitrate_;
        // Stream info (for pipeline)
        StreamInfo source_stream_info_;
        StreamInfo yolo_stream_info_;
        StreamInfo full_stream_info_;
        std::vector<StreamInfo> tracking_stream_infos_;
        // GPU type
        std::string gpu_type_;

    private: // Data
        GstElement* pipeline_;
        std::vector<GstElement*> crops_;
        bool running_;

    private: // Callbacks
        void controlCallback(const core::StringMsg::SharedPtr msg);

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
    };

} // namespace flychams::agent