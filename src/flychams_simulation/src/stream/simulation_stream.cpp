#include "flychams_simulation/stream/simulation_stream.hpp"

using namespace flychams::common;

using namespace flychams::simulation;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void SimulationStream::onModuleInit()
{
    // Get stream parameters
    stream_delay_ms_ = node_->getParameterOr<int>("stream_delay_ms", 500);
    // RTSP server parameters
    rtsp_host_ = node_->getParameterOr<std::string>("rtsp_host", "localhost");
    rtsp_port_ = node_->getParameterOr<int>("rtsp_port", 8554);
    // Scenario view resolution
    scenario_width_  = node_->getParameterOr<int>("scenario_view.width", 1280);
    scenario_height_ = node_->getParameterOr<int>("scenario_view.height", 720);
    // Agent view resolution
    agent_width_  = node_->getParameterOr<int>("agent_view.width", 854);
    agent_height_ = node_->getParameterOr<int>("agent_view.height", 480);
    // Payload view resolution
    payload_width_  = node_->getParameterOr<int>("payload_view.width", 854);
    payload_height_ = node_->getParameterOr<int>("payload_view.height", 480);

    // Get hardware vendor from environment variable
    hw_vendor_ = std::getenv("HW_VENDOR") ? std::getenv("HW_VENDOR") : "none";

    // Base RTSP URL: rtsp://<host>:<port>
    const std::string rtsp_base = "rtsp://" + rtsp_host_ + ":" + std::to_string(rtsp_port_);

    // Initialize stream variables
    stream_units_.clear();

    // Launch per-agent view streams
    // URL format: rtsp://<host>:<port>/<VehicleName>/<CameraName>
    const AgentTeamConfig& agent_team = node_->getSettings()->getAgentTeam();
    bool scenario_launched = false;
    for (const auto& [agent_id, agent_ptr] : agent_team)
    {
        const std::string agent_base = rtsp_base + "/" + agent_id;

        // SCENARIOCAM is mounted on the first agent only
        if (!scenario_launched)
        {
            auto unit = std::make_shared<StreamUnit>();
            unit->camera_id     = "SCENARIOCAM";
            unit->view_id       = "scenario";
            unit->pipeline      = agent_base + "/SCENARIOCAM";
            unit->output_width  = scenario_width_;
            unit->output_height = scenario_height_;
            unit->image_pub     = node_->createSimulationImagePublisher("scenario");
            unit->running       = true;
            unit->thread        = std::thread(&SimulationStream::streamPipeline, this, unit);
            stream_units_["SCENARIOCAM"] = unit;

            if (stream_delay_ms_ > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(stream_delay_ms_));

            scenario_launched = true;
        }

        {
            const ID cam_id = "AGENTCAM_" + agent_id;
            auto unit = std::make_shared<StreamUnit>();
            unit->camera_id     = cam_id;
            unit->view_id       = agent_id + "/body";
            unit->pipeline      = agent_base + "/AGENTCAM_" + agent_id;
            unit->output_width  = agent_width_;
            unit->output_height = agent_height_;
            unit->image_pub     = node_->createSimulationImagePublisher(agent_id + "/body");
            unit->running       = true;
            unit->thread        = std::thread(&SimulationStream::streamPipeline, this, unit);
            stream_units_[cam_id] = unit;

            if (stream_delay_ms_ > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(stream_delay_ms_));
        }

        {
            const ID cam_id = "PAYLOADCAM_" + agent_id;
            auto unit = std::make_shared<StreamUnit>();
            unit->camera_id     = cam_id;
            unit->view_id       = agent_id + "/payload";
            unit->pipeline      = agent_base + "/PAYLOADCAM_" + agent_id;
            unit->output_width  = payload_width_;
            unit->output_height = payload_height_;
            unit->image_pub     = node_->createSimulationImagePublisher(agent_id + "/payload");
            unit->running       = true;
            unit->thread        = std::thread(&SimulationStream::streamPipeline, this, unit);
            stream_units_[cam_id] = unit;

            if (stream_delay_ms_ > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(stream_delay_ms_));
        }
    }
}

void SimulationStream::onModuleShutdown()
{
    for (auto& [id, unit] : stream_units_)
    {
        unit->running = false;
    }
    for (auto& [id, unit] : stream_units_)
    {
        if (unit->thread.joinable())
        {
            unit->thread.join();
        }
    }
    stream_units_.clear();
}

// ════════════════════════════════════════════════════════════════════════════
// STREAM CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════

std::string SimulationStream::buildSourcePipeline(const std::string& rtsp_url) const
{
    const std::string source =
        "rtspsrc location=" + rtsp_url + " latency=0 protocols=tcp timeout=5000000 "
        "! rtph265depay ! h265parse ";

    if (hw_vendor_ == "nvidia")
    {
        // NVDEC hardware-accelerated H.265 decode
        return source +
            "! nvh265dec ! videoconvert ! video/x-raw,format=BGR "
            "! appsink drop=true max-buffers=1 sync=false";
    }
    else if (hw_vendor_ == "amd")
    {
        // VA-API hardware-accelerated H.265 decode
        return source +
            "! vah265dec ! vapostproc ! videoconvert ! video/x-raw,format=BGR "
            "! appsink drop=true max-buffers=1 sync=false";
    }
    else
    {
        // Software decode H.265 decode
        return source +
            "! avdec_h265 ! videoconvert ! video/x-raw,format=BGR "
            "! appsink drop=true max-buffers=1 sync=false";
    }
}

// ════════════════════════════════════════════════════════════════════════════
// STREAM MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void SimulationStream::streamPipeline(const std::shared_ptr<StreamUnit>& unit)
{
    cv::VideoCapture capture;
    cv::Mat frame, low_res_frame;

    RCLCPP_INFO(node_->get_logger(), "Simulation stream: Opening stream for camera %s: %s",
        unit->camera_id.c_str(), unit->pipeline.c_str());

    const std::string gst_pipeline = buildSourcePipeline(unit->pipeline);
    RCLCPP_INFO(node_->get_logger(), "Simulation stream: GStreamer pipeline: %s", gst_pipeline.c_str());

    while (unit->running && !capture.isOpened())
    {
        capture.open(gst_pipeline, cv::CAP_GSTREAMER);
        if (!capture.isOpened())
        {
            capture.release();
            RCLCPP_WARN(node_->get_logger(), "Simulation stream: Could not open stream for camera %s, retrying in 5s...",
                unit->camera_id.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    if (!capture.isOpened())
        return;

    RCLCPP_INFO(node_->get_logger(), "Simulation stream: Stream opened for camera %s",
        unit->camera_id.c_str());

    while (unit->running)
    {
        if (!capture.read(frame) || frame.empty())
            break;
        
        // Downscale frame
        cv::resize(frame, low_res_frame, cv::Size(unit->output_width, unit->output_height), 0, 0, cv::INTER_NEAREST);
        auto img_msg = makeImage(low_res_frame, unit->view_id);
        unit->image_pub.publish(img_msg);
    }

    capture.release();
}

// ════════════════════════════════════════════════════════════════════════════
// IMAGE UTILITIES
// ════════════════════════════════════════════════════════════════════════════

ImageMsg::SharedPtr SimulationStream::makeImage(const cv::Mat& image, const std::string& frame_id) const
{
    std_msgs::msg::Header header;
    header.stamp = node_->now();
    header.frame_id = frame_id;
    return cv_bridge::CvImage(header, "bgr8", image).toImageMsg();
}