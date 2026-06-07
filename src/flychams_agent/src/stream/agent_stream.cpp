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
            unit->output_width = central_view_width;
            unit->output_height = central_view_height;

            // Configure multi-window crop streams associated with this central camera
            int nw = tracking_config.multi_window_set.size();
            if (nw > 0)
            {
                // Create publishers for each crop window
                for (const auto& [window_id, _] : tracking_config.multi_window_set)
                {
                    unit->crop_pubs.push_back(node_->createCameraPublisher(agent_id_, window_id));
                }

                unit->enable_crops = true;
                unit->crops.resize(nw);
                unit->crops_cache.resize(nw);
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

        // Create publisher for this camera
        unit->camera_info = makeCameraInfo(camera, camera->camera.resolution(0), camera->camera.resolution(1), camera->ref_focal);
        unit->image_pub = node_->createCameraPublisher(agent_id_, camera_id);
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
    const size_t n = msg->ids.size();
    for (size_t i = 0; i < n; ++i)
    {
        const auto& camera_id = msg->ids[i];
        auto it = stream_units_.find(camera_id);
        if (it == stream_units_.end())
            continue;

        auto& unit = it->second;
        const auto role = static_cast<ObservationRole>(msg->roles[i]);
        const auto type = static_cast<ObservationType>(msg->types[i]);

        // Update camera_info focal length for tracking camera units
        if (type == ObservationType::Camera && role != ObservationRole::Central)
        {
            const float focal = msg->focals[i];
            std::lock_guard<std::mutex> lock(unit->camera_info_mutex);
            unit->camera_info = makeCameraInfo(unit->config, unit->output_width, unit->output_height, focal);
        }

        // Update crops for the central camera windows
        if (role == ObservationRole::Central && unit->enable_crops)
        {
            const size_t nw = msg->crops.size() - 1;
            for (size_t j = 0, k = 1; j < nw; ++j, ++k)
            {
                const auto& crop = msg->crops[k];
                if (!crop.is_out_of_bounds)
                {
                    std::lock_guard<std::mutex> lock(unit->crops_mutex);
                    unit->crops[j] = crop;
                }
            }
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// STREAM CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════

std::string AgentStream::buildSourcePipeline(const std::string& rtsp_url) const
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

void AgentStream::streamPipeline(const std::shared_ptr<StreamUnit>& unit)
{
    cv::VideoCapture capture;
    cv::Mat frame, low_res_frame, low_res_crop;

    RCLCPP_INFO(node_->get_logger(), "Agent stream: Opening stream for camera %s: %s",
        unit->config->id.c_str(), unit->pipeline.c_str());

    const std::string gst_pipeline = buildSourcePipeline(unit->pipeline);
    RCLCPP_INFO(node_->get_logger(), "Agent stream: GStreamer pipeline: %s", gst_pipeline.c_str());

    while (unit->running && !capture.isOpened())
    {
        capture.open(gst_pipeline, cv::CAP_GSTREAMER);
        if (!capture.isOpened())
        {
            capture.release();
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

        // Downscale frame
        cv::resize(frame, low_res_frame, cv::Size(unit->output_width, unit->output_height), 0, 0, cv::INTER_AREA);
        auto img_msg = makeImage(low_res_frame, unit->frame_id);
        CameraInfoMsg ci_snapshot;
        {
            std::lock_guard<std::mutex> lock(unit->camera_info_mutex);
            ci_snapshot = unit->camera_info;
        }
        auto ci_msg = std::make_shared<CameraInfoMsg>(ci_snapshot);
        ci_msg->header = img_msg->header;
        unit->image_pub.publish(img_msg, ci_msg);

        if (unit->enable_crops)
        {
            // Update crops cache only when the mutex is immediately available
            if (unit->crops_mutex.try_lock())
            {
                unit->crops_cache = unit->crops;
                unit->crops_mutex.unlock();
            }

            // Get and create crop rectangles
            const cv::Rect frame_rect(0, 0, frame.cols, frame.rows);
            const size_t n_crops = std::min(unit->crops_cache.size(), unit->crop_pubs.size());
            for (size_t i = 0; i < n_crops; i++)
            {
                const auto& crop = unit->crops_cache[i];

                cv::Rect rect(crop.x, crop.y, crop.w, crop.h);
                rect &= frame_rect;
                if (rect.width <= 0 || rect.height <= 0)
                    continue;

                // Scale crop
                cv::resize(frame(rect), low_res_crop, cv::Size(unit->crop_output_width, unit->crop_output_height), 0, 0, cv::INTER_AREA);
                auto crop_img = makeImage(low_res_crop, unit->frame_id);
                auto crop_ci = std::make_shared<CameraInfoMsg>(unit->camera_info);
                crop_ci->header = crop_img->header;
                crop_ci->width = unit->crop_output_width;
                crop_ci->height = unit->crop_output_height;
                unit->crop_pubs[i].publish(crop_img, crop_ci);
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

CameraInfoMsg AgentStream::makeCameraInfo(const MultiCameraConfigPtr& config, int width, int height, float focal) const
{
    CameraInfoMsg ci;
    ci.width  = static_cast<uint32_t>(width);
    ci.height = static_cast<uint32_t>(height);
    ci.distortion_model = "plumb_bob";

    const auto& cam   = config->camera;
    const float sw    = cam.sensor_size(0);
    const float sh    = cam.sensor_size(1);
    const float rho_x = sw / static_cast<float>(cam.resolution(0));
    const float rho_y = sh / static_cast<float>(cam.resolution(1));
    const float fx    = focal / rho_x;
    const float fy    = focal / rho_y;
    const float cx    = static_cast<float>(cam.resolution(0)) / 2.0f;
    const float cy    = static_cast<float>(cam.resolution(1)) / 2.0f;

    ci.k = {fx, 0.0, cx,
             0.0, fy, cy,
             0.0, 0.0, 1.0};
    ci.r = {1.0, 0.0, 0.0,
             0.0, 1.0, 0.0,
             0.0, 0.0, 1.0};
    ci.p = {fx, 0.0, cx, 0.0,
             0.0, fy, cy, 0.0,
             0.0, 0.0, 1.0, 0.0};
    ci.d = {static_cast<double>(cam.distortion.K1),
             static_cast<double>(cam.distortion.K2),
             static_cast<double>(cam.distortion.P1),
             static_cast<double>(cam.distortion.P2),
             static_cast<double>(cam.distortion.K3)};
    return ci;
}