#pragma once

// Standard includes
#include <atomic>
#include <mutex>
#include <thread>

// OpenCV includes
#include <cv_bridge/cv_bridge.h>

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Class to handle simulation view streaming via RTSP → ROS2
     *
     * @details
     * Bridges AirSim RTSP streams for simulation view cameras
     * (SCENARIOCAM, AGENTCAM_<id>, PAYLOADCAM_<id>) to ROS2
     * image_transport topics, mirroring AgentStream for sim views.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-21
     * ════════════════════════════════════════════════════════════════
     */
    class SimulationStream : public common::BaseModule
    {
    public: // Constructor/Destructor
        explicit SimulationStream(common::BaseNode::SharedPtr node)
            : BaseModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<SimulationStream>;
        struct StreamUnit
        {
            // Unit identification
            common::ID camera_id;
            common::ID view_id;
            // Pipeline
            std::string pipeline;
            // Output dimensions
            int output_width;
            int output_height;
            // Publisher
            common::ImagePublisher image_pub;
            // Runtime
            std::atomic_bool running;
            std::thread thread;
            // Constructor
            StreamUnit()
                : camera_id(), view_id(), pipeline(),
                  output_width(0), output_height(0), image_pub(), running(false), thread()
            {
            }
        };

    private: // Parameters
        int stream_delay_ms_;
        std::string hw_vendor_;
        // RTSP server
        std::string rtsp_host_;
        int rtsp_port_;
        // Scenario view resolution
        int scenario_width_;
        int scenario_height_;
        // Agent view resolution
        int agent_width_;
        int agent_height_;
        // Payload view resolution
        int payload_width_;
        int payload_height_;

    private: // Data
        // Stream units keyed by camera_id
        std::unordered_map<common::ID, std::shared_ptr<StreamUnit>> stream_units_;

    private: // Stream configuration
        std::string buildSourcePipeline(const std::string& rtsp_url) const;

    private: // Stream management
        void streamPipeline(const std::shared_ptr<StreamUnit>& unit);

    private: // Image utilities
        common::ImageMsg::SharedPtr makeImage(const cv::Mat& image, const std::string& frame_id) const;
    };

} // namespace flychams::simulation