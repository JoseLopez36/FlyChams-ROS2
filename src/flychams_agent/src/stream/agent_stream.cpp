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
    jpeg_quality_ = node_->getParameterOr<int>("jpeg_quality", 80);
    rtsp_latency_ms_ = node_->getParameterOr<int>("rtsp_latency_ms", 100);
    reconnect_delay_ms_ = node_->getParameterOr<int>("reconnect_delay_ms", 2000);
    stream_start_delay_ms_ = node_->getParameterOr<int>("stream_start_delay_ms", 500);
    output_encoding_ = node_->getParameterOr<std::string>("output_encoding", "jpg");
    gpu_vendor_ = node_->getParameterOr<std::string>("gpu_vendor", "auto");
    
    // Auto-detect GPU vendor if set to "auto"
    if (gpu_vendor_ == "auto")
    {
        const char* cuda_visible = std::getenv("CUDA_VISIBLE_DEVICES");
        const char* rocr_visible = std::getenv("ROCR_VISIBLE_DEVICES");
        if (cuda_visible && strlen(cuda_visible) > 0)
            gpu_vendor_ = "nvidia";
        else if (rocr_visible && strlen(rocr_visible) > 0)
            gpu_vendor_ = "amd";
        else
            gpu_vendor_ = "none";
    }

    // Initialize stream variables
    stream_units_.clear();

    // Get observation units config
    const TrackingConfig& tracking_config = node_->getSettings()->getTracking(agent_id_);
    int stream_index = 0;
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
        unit->start_delay_ms = stream_index * stream_start_delay_ms_;
        unit->running = true;
        unit->thread = std::thread(&AgentStream::streamPipeline, this, unit);
        stream_units_[camera_id] = unit;
        stream_index++;
    }

    // Set OpenCV FFMPEG capture options based on GPU vendor
    std::string ffmpeg_options = "rtsp_transport;tcp";
    if (gpu_vendor_ == "nvidia")
    {
        ffmpeg_options = "video_codec;hevc_cuvid|rtsp_transport;tcp";
    }
    else if (gpu_vendor_ == "amd" || gpu_vendor_ == "intel")
    {
        ffmpeg_options = "video_codec;hevc_vaapi|rtsp_transport;tcp";
    }
    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", ffmpeg_options.c_str(), 1);

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
// STREAM MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void AgentStream::streamPipeline(const std::shared_ptr<StreamUnit>& unit)
{
    cv::VideoCapture capture;
    cv::Mat frame;

    if (unit->start_delay_ms > 0 && unit->running)
        std::this_thread::sleep_for(std::chrono::milliseconds(unit->start_delay_ms));

    while (unit->running)
    {
        RCLCPP_INFO(node_->get_logger(), "Agent stream: Attempting to open stream for camera %s: %s",
            unit->config->id.c_str(), unit->pipeline.c_str());

        capture.open(unit->pipeline, cv::CAP_FFMPEG);

        if (!capture.isOpened())
        {
            RCLCPP_ERROR(node_->get_logger(), "Agent stream: Could not open stream for camera %s, retrying in %d ms...",
                unit->config->id.c_str(), reconnect_delay_ms_);
            capture.release();
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay_ms_));
            continue;
        }

        RCLCPP_INFO(node_->get_logger(), "Agent stream: Stream opened for camera %s", unit->config->id.c_str());
        capture.set(cv::CAP_PROP_BUFFERSIZE, 1);

        while (unit->running)
        {
            // Move from GPU to CPU through PCIe (very fast)
            bool success = capture.read(frame);

            // Check frame validity
            if (!success || frame.empty())
            {
                RCLCPP_WARN(node_->get_logger(), "Agent stream: Stream lost for camera %s, reconnecting in %d ms...",
                    unit->config->id.c_str(), reconnect_delay_ms_);
                capture.release();
                std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay_ms_));
                break;
            }

            // Downscale on CPU (nearest neighbor for speed)
            cv::Mat low_res_frame;
            cv::resize(frame, low_res_frame, cv::Size(unit->output_width, unit->output_height), 0, 0, cv::INTER_NEAREST);
            unit->image_pub->publish(makeCompressedImage(low_res_frame, unit->frame_id));

            // Crop on CPU (zero-copy in RAM)
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
                    rect = rect & cv::Rect(0, 0, frame.cols, frame.rows);
                    if (rect.width <= 0 || rect.height <= 0)
                    {
                        continue;
                    }

                    cv::Mat crop_frame = frame(rect);
                    cv::Mat low_res_crop;
                    cv::resize(crop_frame, low_res_crop, cv::Size(unit->crop_output_width, unit->crop_output_height), 0, 0, cv::INTER_NEAREST);
                    unit->crop_pubs[i]->publish(makeCompressedImage(low_res_crop, unit->frame_id));
                }
            }
        }

        capture.release();
    }
}

// ════════════════════════════════════════════════════════════════════════════
// IMAGE UTILITIES
// ════════════════════════════════════════════════════════════════════════════

CompressedImageMsg AgentStream::makeCompressedImage(const cv::Mat& image, const std::string& frame_id) const
{
    CompressedImageMsg msg;
    msg.header.stamp = node_->now();
    msg.header.frame_id = frame_id;
    msg.format = output_encoding_;

    std::vector<int> params;
    std::string extension = "." + output_encoding_;

    if (output_encoding_ == "jpg" || output_encoding_ == "jpeg")
    {
        params = {cv::IMWRITE_JPEG_QUALITY, std::clamp(jpeg_quality_, 1, 100)};
        extension = ".jpg";
        msg.format = "jpeg";
    }

    cv::imencode(extension, image, msg.data, params);

    return msg;
}