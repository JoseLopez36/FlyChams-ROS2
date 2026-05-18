#include "flychams_agent/stream/agent_stream.hpp"

using namespace flychams::common;

using namespace flychams::agent;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void AgentStream::onModuleInit()
{
    // Get parameters from parameter server
    // Interface parameters
    central_view_width = node_->getParameterOr<int>("central_view.width", 854);
    central_view_height = node_->getParameterOr<int>("central_view.height", 480);
    tracking_view_width = node_->getParameterOr<int>("tracking_view.width", 427);
    tracking_view_height = node_->getParameterOr<int>("tracking_view.height", 240);
    // Stream parameters
    stream_delay_ms_ = node_->getParameterOr<int>("stream_delay_ms", 500);

    // Get hardware vendor from environment variable
    hw_vendor_ = std::getenv("HW_VENDOR") ? std::getenv("HW_VENDOR") : "none";

    // Initialize stream variables
    stream_units_.clear();

    // Get observation units config
    const TrackingConfig& tracking_config = node_->getSettings()->getTracking(agent_id_);
    for (const auto& [camera_id, camera] : tracking_config.multi_camera_set)
    {
        std::shared_ptr<StreamUnit> unit = std::make_shared<StreamUnit>();
        unit->config = camera;
        unit->pipeline = camera->source_stream_url;
        unit->frame_id = node_->getCameraOpticalFrame(agent_id_, camera_id);
        if (camera->role == ObservationRole::Central)
        {
            central_camera_id_ = camera_id;

            unit->output_width = central_view_width;
            unit->output_height = central_view_height;

            // Configure multi-window
            int nw = tracking_config.multi_window_set.size();
            if (nw > 0)
            {
                unit->enable_crops = true;
                unit->crops.resize(nw);
                unit->crops_cache.resize(nw);
                for (const auto& [window_id, window] : tracking_config.multi_window_set)
                {
                    unit->crop_pubs.push_back(node_->createAgentImagePublisher(agent_id_, window_id));
                }
                unit->crop_output_width = tracking_view_width;
                unit->crop_output_height = tracking_view_height;
            }
        }
        else if (camera->role == ObservationRole::Tracking)
        {
            unit->output_width = tracking_view_width;
            unit->output_height = tracking_view_height;
            unit->enable_crops = false;
        }

        unit->image_pub = node_->createAgentImagePublisher(agent_id_, camera_id);
        unit->running = true;
        unit->thread = std::thread(&AgentStream::streamPipeline, this, unit);
        stream_units_[camera_id] = unit;

        // Delay before starting next stream
        if (stream_delay_ms_ > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(stream_delay_ms_));
        }
    }

    // Subscribe to GUI setpoints topic
    observation_setpoints_sub_ = node_->createObservationSetpointsSubscriber(agent_id_,
        std::bind(&AgentStream::observationSetpointsCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

}

void AgentStream::onModuleShutdown()
{
    // Destroy subscribers
    observation_setpoints_sub_.reset();

    // Stop stream units
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
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void AgentStream::observationSetpointsCallback(const ObservationSetpointsMsg::SharedPtr msg)
{
    // Get central stream unit
    std::shared_ptr<StreamUnit> unit = stream_units_[central_camera_id_];

    // Check if crops are enabled
    if (!unit->enable_crops)
    {
        return;
    }

    // Iterate through the crops in the message
    const size_t nw = msg->crops.size() - 1; // Exclude the central camera
    for (size_t i = 0, j = 1; i < nw; ++i, ++j)
    {
        const auto& crop = msg->crops[j];
        
        // Only update if it's not out of bounds
        if (!crop.is_out_of_bounds)
        {
            std::lock_guard<std::mutex> lock(unit->crops_mutex);
            unit->crops[i] = crop;
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// STREAM CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════

std::string AgentStream::buildSourcePipeline(const std::string& rtsp_url) const
{
    const std::string source =
        "rtspsrc location=" + rtsp_url + " latency=0 protocols=tcp "
        "! rtph265depay ! h265parse ";

    if (hw_vendor_ == "nvidia")
    {
        // NVDEC hardware-accelerated H.265 decode
        return source +
            "! nvv4l2decoder ! nvvidconv ! video/x-raw,format=BGRx "
            "! videoconvert ! video/x-raw,format=BGR "
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

void AgentStream::streamPipeline(const std::shared_ptr<StreamUnit>& unit)
{
    cv::VideoCapture capture;
    cv::Mat  frame, low_res_frame, low_res_crop;

    RCLCPP_INFO(node_->get_logger(), "Agent stream: Opening stream for camera %s: %s",
        unit->config->id.c_str(), unit->pipeline.c_str());

    const std::string gst_pipeline = buildSourcePipeline(unit->pipeline);
    RCLCPP_INFO(node_->get_logger(), "Agent stream: GStreamer pipeline: %s", gst_pipeline.c_str());

    while (unit->running && !capture.isOpened())
    {
        capture.open(gst_pipeline, cv::CAP_GSTREAMER);
        if (!capture.isOpened())
        {
            RCLCPP_WARN(node_->get_logger(), "Agent stream: Could not open stream for camera %s, retrying in 5s...",
                unit->config->id.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    if (!capture.isOpened())
        return;

    RCLCPP_INFO(node_->get_logger(), "Agent stream: Stream opened for camera %s",
        unit->config->id.c_str());

    while (unit->running)
    {
        if (!capture.read(frame) || frame.empty())
            break;

        // Downscale: INTER_AREA gives best quality for shrinking and avoids aliasing
        cv::resize(frame, low_res_frame, cv::Size(unit->output_width, unit->output_height), 0, 0, cv::INTER_AREA);
        unit->image_pub.publish(makeImage(low_res_frame, unit->frame_id));

        if (unit->enable_crops)
        {
            // Update crops cache only when the mutex is immediately available
            if (unit->crops_mutex.try_lock())
            {
                unit->crops_cache = unit->crops;
                unit->crops_mutex.unlock();
            }

            const cv::Rect frame_rect(0, 0, frame.cols, frame.rows);
            const size_t n_crops = std::min(unit->crops_cache.size(), unit->crop_pubs.size());
            for (size_t i = 0; i < n_crops; i++)
            {
                const auto& crop = unit->crops_cache[i];

                cv::Rect rect(crop.x, crop.y, crop.w, crop.h);
                rect &= frame_rect;
                if (rect.width <= 0 || rect.height <= 0)
                    continue;

                cv::resize(frame(rect), low_res_crop, cv::Size(unit->crop_output_width, unit->crop_output_height), 0, 0, cv::INTER_AREA);
                unit->crop_pubs[i].publish(makeImage(low_res_crop, unit->frame_id));
            }
        }
    }

    capture.release();
}

// ════════════════════════════════════════════════════════════════════════════
// IMAGE UTILITIES
// ════════════════════════════════════════════════════════════════════════════

ImageMsg::SharedPtr AgentStream::makeImage(const cv::Mat& image, const std::string& frame_id) const
{
    std_msgs::msg::Header header;
    header.stamp = node_->now();
    header.frame_id = frame_id;
    return cv_bridge::CvImage(header, "bgr8", image).toImageMsg();
}