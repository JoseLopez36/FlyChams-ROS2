#pragma once

// Standard includes
#include <atomic>
#include <mutex>
#include <thread>

// OpenCV includes
#include <cv_bridge/cv_bridge.h>

// Utils include
#include "flychams_common/utils/stream_utils.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Class to handle video streaming using FFmpeg
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-03-06
     * ════════════════════════════════════════════════════════════════
     */
    class AgentStream : public common::BaseModule
    {
    public: // Constructor/Destructor
        AgentStream(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentStream>;
        struct StreamUnit
        {
            // Unit configuration
            common::MultiCameraConfigPtr config;
            std::string pipeline;
            std::string frame_id;
            int output_width;
            int output_height;
            bool enable_crops;
            // Crop data
            std::vector<common::CropMsg> crops;
            std::vector<common::CropMsg> crops_cache;
            int crop_output_width;
            int crop_output_height;
            std::mutex crops_mutex;
            // Camera info
            common::CameraInfoMsg camera_info;
            std::mutex camera_info_mutex;
            // Publisher
            common::CameraPublisher image_pub;
            std::vector<common::CameraPublisher> crop_pubs;
            // Runtime
            std::atomic_bool running;
            std::thread thread;
            // Constructor
            StreamUnit()
                : config(), pipeline(), frame_id(), output_width(0), output_height(0), enable_crops(false),
                crops(), crops_mutex(), camera_info(), camera_info_mutex(), image_pub(), crop_pubs(), running(false), thread()
            {
            }
        };
        
    private: // Parameters
        common::ID agent_id_;
        // Interface parameters
        int central_view_width;
        int central_view_height;
        int tracking_view_width;
        int tracking_view_height;
        // Stream parameters
        int stream_delay_ms_;
        std::string hw_vendor_;

    private: // Data
        // Stream units
        std::unordered_map<common::ID, std::shared_ptr<StreamUnit>> stream_units_;

    private: // Callbacks
        void observationSetpointsCallback(const common::ObservationSetpointsMsg::SharedPtr msg);

    private: // Stream management
        void streamPipeline(const std::shared_ptr<StreamUnit>& unit);

    private: // Image utilities
        common::ImageMsg::SharedPtr makeImage(const cv::Mat& image, const std::string& frame_id) const;
        common::CameraInfoMsg makeCameraInfo(const common::MultiCameraConfigPtr& config, int width, int height, float focal) const;

    private: // ROS components
        // Subscriber
        common::SubscriberPtr<common::ObservationSetpointsMsg> observation_setpoints_sub_;
    };

} // namespace flychams::agent