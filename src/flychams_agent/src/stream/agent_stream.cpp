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
                for (const auto& [window_id, window] : tracking_config.multi_window_set)
                {
                    unit->crop_pubs.push_back(node_->createAgentMultiWindowImagePublisher(agent_id_, window_id));
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

        unit->image_pub = node_->createAgentMultiCameraImagePublisher(agent_id_, camera_id);
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

    // Iterate through the crops in the message
    const size_t n = msg->crops.size();
    for (size_t i = 0; i < n; ++i)
    {
        const auto& crop = msg->crops[i];
        
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
    cv::UMat gpu_frame, gpu_low_res, gpu_low_res_crop;
    cv::Mat  low_res_frame, low_res_crop;

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
        if (!capture.read(gpu_frame) || gpu_frame.empty())
            break;

        // Downscale on GPU, download only the small result
        cv::resize(gpu_frame, gpu_low_res, cv::Size(unit->output_width, unit->output_height));
        gpu_low_res.copyTo(low_res_frame);
        unit->image_pub.publish(makeImage(low_res_frame, unit->frame_id));

        // Crop on GPU, resize on GPU, download only the small result
        if (unit->enable_crops)
        {
            std::vector<CropMsg> crops;
            {
                std::lock_guard<std::mutex> lock(unit->crops_mutex);
                crops = unit->crops;
            }

            for (size_t i = 0; i < crops.size(); i++)
            {
                const auto& crop = crops[i];

                cv::Rect rect(crop.x, crop.y, crop.w, crop.h);
                rect = rect & cv::Rect(0, 0, gpu_frame.cols, gpu_frame.rows);
                if (rect.width <= 0 || rect.height <= 0)
                    continue;

                cv::resize(gpu_frame(rect), gpu_low_res_crop, cv::Size(unit->crop_output_width, unit->crop_output_height));
                gpu_low_res_crop.copyTo(low_res_crop);
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