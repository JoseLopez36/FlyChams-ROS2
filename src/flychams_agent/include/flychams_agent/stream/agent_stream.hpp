#pragma once

// Standard includes
#include <atomic>
#include <mutex>
#include <thread>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_map>

// OpenCV include
#include <opencv2/opencv.hpp>

// GStreamer include
#include <gst/gst.h>

// Base module include
#include "flychams_common/base/base_module.hpp"

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
        struct StreamUnit
        {
            // Unit configuration
            core::MultiCameraConfigPtr config;
            std::string pipeline;
            std::string frame_id;
            int output_width;
            int output_height;
            bool enable_crops;
            // Crop data
            std::vector<core::CropMsg> crops;
            int crop_output_width;
            int crop_output_height;
            std::mutex crops_mutex;
            // Publisher
            core::PublisherPtr<core::CompressedImageMsg> image_pub;
            std::vector<core::PublisherPtr<core::CompressedImageMsg>> crop_pubs;
            // Runtime
            std::atomic_bool running;
            std::thread thread;
            // Constructor
            StreamUnit()
                : config(), pipeline(), frame_id(), output_width(0), output_height(0), enable_crops(false),
                crops(), crops_mutex(), image_pub(), crop_pubs(), running(false), thread()
            {
            }
        };
        
    private: // Parameters
        core::ID agent_id_;
        // Interface parameters
        int central_view_width;
        int central_view_height;
        int tracking_view_width;
        int tracking_view_height;
        // Stream parameters
        int jpeg_quality_;
        int rtsp_latency_ms_;
        int reconnect_delay_ms_;
        std::string output_encoding_;

    private: // Data
        // Stream units
        std::unordered_map<core::ID, std::shared_ptr<StreamUnit>> stream_units_;

    private: // Callbacks
        void guiSetpointsCallback(const core::AgentGuiSetpointsMsg::SharedPtr msg);

    private: // Stream configuration
        std::string createPipeline(const core::MultiCameraConfigPtr& camera) const;

    private: // Stream management
        void streamPipeline(const std::shared_ptr<StreamUnit>& unit);

    private: // Image utilities
        core::CompressedImageMsg makeCompressedImage(const cv::Mat& image, const std::string& frame_id) const;

    private: // ROS components
        // Subscriber
        core::SubscriberPtr<core::AgentGuiSetpointsMsg> gui_setpoints_sub_;
    };

} // namespace flychams::agent