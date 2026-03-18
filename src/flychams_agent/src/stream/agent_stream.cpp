#include "flychams_agent/stream/agent_stream.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void AgentStream::onInit()
    {
        // Get parameters from parameter server
        // YOLO stream parameters
        yolo_width_ = RosUtils::getParameterOr<int>(node_, "yolo_stream.width", 640);
        yolo_height_ = RosUtils::getParameterOr<int>(node_, "yolo_stream.height", 640);
        yolo_bitrate_ = RosUtils::getParameterOr<int>(node_, "yolo_stream.bitrate", 1000);
        // Central stream parameters
        central_width_ = RosUtils::getParameterOr<int>(node_, "central_stream.width", 1280);
        central_height_ = RosUtils::getParameterOr<int>(node_, "central_stream.height", 720);
        central_bitrate_ = RosUtils::getParameterOr<int>(node_, "central_stream.bitrate", 3000);
        // Tracking streams parameters
        tracking_width_ = RosUtils::getParameterOr<int>(node_, "tracking_streams.width", 1280);
        tracking_height_ = RosUtils::getParameterOr<int>(node_, "tracking_streams.height", 720);
        tracking_bitrate_ = RosUtils::getParameterOr<int>(node_, "tracking_streams.bitrate", 3000);
        // GPU type
        gpu_type_ = RosUtils::getParameterOr<std::string>(node_, "gpu_type", "auto");

        // Get relevant config
        const AgentConfigPtr& agent_config = settings_tools_->getAgent(agent_id_);
        const TrackingConfig& tracking_config = settings_tools_->getTracking(agent_id_);
        MultiCameraConfigPtr central_camera_config;
        std::vector<MultiCameraConfigPtr> tracking_camera_configs;
        std::vector<MultiWindowConfigPtr> tracking_window_configs;
        source_width_ = 0;
        source_height_ = 0;
        for (const auto& [multi_camera_id, multi_camera] : tracking_config.multi_camera_set)
        {
            if (multi_camera->role == ObservationRole::Central)
            {
                central_camera_config = multi_camera;
                source_width_ = multi_camera->camera.resolution(0);
                source_height_ = multi_camera->camera.resolution(1);
            }
            else if (multi_camera->role == ObservationRole::Tracking)
            {
                tracking_camera_configs.push_back(multi_camera);
            }
        }
        for (const auto& [multi_window_id, multi_window] : tracking_config.multi_window_set)
        {
            tracking_window_configs.push_back(multi_window);
        }

        // Get stream info
        source_stream_info_ = getSourceStreamInfo(central_camera_config);
        yolo_stream_info_ = getYoloStreamInfo(agent_config);
        central_stream_info_ = getInterfaceStreamInfo(central_camera_config);
        tracking_stream_infos_.clear();
        for (const auto& tracking_camera_config : tracking_camera_configs)
        {
            tracking_stream_infos_.push_back(getInterfaceStreamInfo(tracking_camera_config));
        }
        for (const auto& tracking_window_config : tracking_window_configs)
        {
            tracking_stream_infos_.push_back(getInterfaceStreamInfo(tracking_window_config));
        }

        // Detect GPU type if not specified
        if (gpu_type_ == "auto")
        {
            gpu_type_ = detectGpuType();
            RCLCPP_INFO(node_->get_logger(), "Auto-detected GPU type: %s", gpu_type_.c_str());
        }

        // Create pipeline
        const std::string pipeline_str = createPipeline(gpu_type_);

        // Initialize GStreamer
        gst_init(nullptr, nullptr);

        // Start pipeline
        startStream(pipeline_str);

        // Subscribe to GUI setpoints topic
        gui_setpoints_sub_ = topic_tools_->createAgentGuiSetpointsSubscriber(agent_id_,
            std::bind(&AgentStream::guiSetpointsCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
    }

    void AgentStream::onShutdown()
    {
        stopStream();

        // Destroy subscribers
        gui_setpoints_sub_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void AgentStream::guiSetpointsCallback(const core::AgentGuiSetpointsMsg::SharedPtr msg)
    {
        if (!running_)
        {
            return;
        }

        // Iterate through the crops in the message
        for (size_t i = 0; i < msg->crops.size(); ++i)
        {
            const auto& crop = msg->crops[i];

            // Only update if it's not out of bounds
            if (!crop.is_out_of_bounds)
            {
                // Calculate margins for videocrop element
                int left = std::min(std::max(0, (int)crop.x), source_width_);
                int top = std::min(std::max(0, (int)crop.y), source_height_);
                int right = std::min(std::max(0, source_width_ - ((int)crop.x + (int)crop.w)), source_width_);
                int bottom = std::min(std::max(0, source_height_ - ((int)crop.y + (int)crop.h)), source_height_);

                // Log
                RCLCPP_INFO(node_->get_logger(), "Agent stream: Updating crop %zu: left=%d, top=%d, right=%d, bottom=%d", i, left, top, right, bottom);

                // Update videocrop properties
                if (i < croppers_.size() && croppers_[i] != nullptr)
                {
                    g_object_set(croppers_[i],
                        "left", 0,
                        "right", 0,
                        "top", 0,
                        "bottom", 0,
                        NULL);
                }
                else
                {
                    RCLCPP_ERROR(node_->get_logger(), "Agent stream: Crop element %zu is not available", i);
                }
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STREAM CONFIGURATION
    // ════════════════════════════════════════════════════════════════════════════

    AgentStream::StreamInfo AgentStream::getSourceStreamInfo(const core::MultiCameraConfigPtr& camera_config)
    {
        StreamInfo info;
        info.url = camera_config->source_stream_url;
        info.width = source_width_;
        info.height = source_height_;
        info.bitrate = 0;

        parseUrl(info.url, info.protocol, info.host, info.port);

        return info;
    }

    AgentStream::StreamInfo AgentStream::getYoloStreamInfo(const core::AgentConfigPtr& agent_config)
    {
        StreamInfo info;
        info.url = agent_config->inference_stream_url;
        info.width = yolo_width_;
        info.height = yolo_height_;
        info.bitrate = yolo_bitrate_;

        parseUrl(info.url, info.protocol, info.host, info.port);

        return info;
    }

    AgentStream::StreamInfo AgentStream::getInterfaceStreamInfo(const core::MultiCameraConfigPtr& camera_config)
    {
        StreamInfo info;
        info.url = camera_config->interface_stream_url;
        info.width = central_width_;
        info.height = central_height_;
        info.bitrate = central_bitrate_;

        parseUrl(info.url, info.protocol, info.host, info.port);

        return info;
    }

    AgentStream::StreamInfo AgentStream::getInterfaceStreamInfo(const core::MultiWindowConfigPtr& window_config)
    {
        StreamInfo info;
        info.url = window_config->interface_stream_url;
        info.width = tracking_width_;
        info.height = tracking_height_;
        info.bitrate = tracking_bitrate_;

        parseUrl(info.url, info.protocol, info.host, info.port);

        return info;
    }

    void AgentStream::parseUrl(const std::string& url, std::string& protocol, std::string& host, int& port)
    {
        // Parse URL: protocol://host:port
        std::string url_copy = url;
        size_t protocol_end_pos = url_copy.find("://");
        if (protocol_end_pos != std::string::npos)
        {
            protocol = url_copy.substr(0, protocol_end_pos);
            size_t host_start = protocol_end_pos + 3;
            size_t port_start_pos = url_copy.find(':', host_start);
            if (port_start_pos != std::string::npos)
            {
                host = url_copy.substr(host_start, port_start_pos - host_start);
                try
                {
                    port = std::stoi(url_copy.substr(port_start_pos + 1));
                }
                catch (...)
                {
                    port = 0;
                }
            }
            else
            {
                host = url_copy.substr(host_start);
                port = 0;
            }
        }
        else
        {
            protocol = "";
            host = "";
            port = 0;
        }
    }

    std::string AgentStream::detectGpuType()
    {
        GstRegistry* registry = gst_registry_get();

        if (gst_registry_find_feature(registry, "nvh265enc", GST_TYPE_ELEMENT_FACTORY))
        {
            return "nvidia";
        }

        if (gst_registry_find_feature(registry, "vah265enc", GST_TYPE_ELEMENT_FACTORY))
        {
            return "amd";
        }

        RCLCPP_WARN(node_->get_logger(), "No hardware acceleration found. Defaulting to NVIDIA (NVENC) pipeline");
        return "nvidia";
    }

    std::string AgentStream::createPipeline(const std::string& gpu_type)
    {
        if (gpu_type == "nvidia")
        {
            return createNvidiaPipeline();
        }
        else
        {
            return createAmdPipeline();
        }
    }

    std::string AgentStream::createNvidiaPipeline()
    {
        std::string pipeline_str;

        // Source branch
        if (source_stream_info_.protocol == "udp")
        {
            pipeline_str =
                "udpsrc port=" + std::to_string(source_stream_info_.port) + " buffer-size=5242880 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265, payload=(int)96\" ! "
                "rtph265depay ! h265parse ! nvh265dec ! queue max-size-buffers=1 ! tee name=t ";
        }
        else if (source_stream_info_.protocol == "rtsp")
        {
            pipeline_str =
                "rtspsrc location=" + source_stream_info_.url + " latency=100 ! "
                "rtph265depay ! h265parse ! nvh265dec ! queue max-size-buffers=1 ! tee name=t ";
        }
        else
        {
            RCLCPP_ERROR(node_->get_logger(), "Agent stream: Unsupported source protocol: %s", source_stream_info_.protocol.c_str());
            return "";
        }

        // YOLO branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videoscale ! video/x-raw,width=" + std::to_string(yolo_stream_info_.width) + ",height=" + std::to_string(yolo_stream_info_.height) + " ! nvh264enc bitrate=" + std::to_string(yolo_stream_info_.bitrate) + " rc-mode=cbr ! h264parse config-interval=-1 ! tcpserversink host=" + yolo_stream_info_.host + " port=" + std::to_string(yolo_stream_info_.port) + " sync=false ";

        // Full res branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videoscale ! video/x-raw,width=" + std::to_string(central_stream_info_.width) + ",height=" + std::to_string(central_stream_info_.height) + " ! nvh264enc bitrate=" + std::to_string(central_stream_info_.bitrate) + " rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + central_stream_info_.host + " port=" + std::to_string(central_stream_info_.port) + " sync=false ";

        // Crop branches
        for (size_t i = 0; i < tracking_stream_infos_.size(); ++i)
        {
            const auto& info = tracking_stream_infos_[i];
            std::string crop_name = "crop_" + std::to_string(i);
            pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videocrop name=" + crop_name + " ! videoscale ! video/x-raw,width=" + std::to_string(info.width) + ",height=" + std::to_string(info.height) + " ! nvh264enc bitrate=" + std::to_string(info.bitrate) + " rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + info.host + " port=" + std::to_string(info.port) + " sync=false ";
        }

        return pipeline_str;
    }

    std::string AgentStream::createAmdPipeline()
    {
        std::string pipeline_str;

        // Source branch
        if (source_stream_info_.protocol == "udp")
        {
            pipeline_str =
                "udpsrc port=" + std::to_string(source_stream_info_.port) + " buffer-size=5242880 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265, payload=(int)96\" ! "
                "rtph265depay ! h265parse ! vah265dec ! queue max-size-buffers=1 ! tee name=t ";
        }
        else if (source_stream_info_.protocol == "rtsp")
        {
            pipeline_str =
                "rtspsrc location=" + source_stream_info_.url + " latency=100 ! "
                "rtph265depay ! h265parse ! vah265dec ! queue max-size-buffers=1 ! tee name=t ";
        }
        else
        {
            RCLCPP_ERROR(node_->get_logger(), "Agent stream: Unsupported source protocol: %s", source_stream_info_.protocol.c_str());
            return "";
        }

        // YOLO branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! vapostproc ! video/x-raw,width=" + std::to_string(yolo_stream_info_.width) + ",height=" + std::to_string(yolo_stream_info_.height) + " ! vah264enc bitrate=" + std::to_string(yolo_stream_info_.bitrate) + " ! h264parse config-interval=-1 ! tcpserversink host=" + yolo_stream_info_.host + " port=" + std::to_string(yolo_stream_info_.port) + " sync=false ";

        // Full res branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! vapostproc ! video/x-raw,width=" + std::to_string(central_stream_info_.width) + ",height=" + std::to_string(central_stream_info_.height) + " ! vah264enc bitrate=" + std::to_string(central_stream_info_.bitrate) + " rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + central_stream_info_.host + " port=" + std::to_string(central_stream_info_.port) + " sync=false ";

        // Crop branches
        for (size_t i = 0; i < tracking_stream_infos_.size(); ++i)
        {
            const auto& info = tracking_stream_infos_[i];
            std::string crop_name = "crop_" + std::to_string(i);
            pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videocrop name=" + crop_name + " ! vapostproc ! video/x-raw,width=" + std::to_string(info.width) + ",height=" + std::to_string(info.height) + " ! vah264enc bitrate=" + std::to_string(info.bitrate) + " rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + info.host + " port=" + std::to_string(info.port) + " sync=false ";
        }

        return pipeline_str;
    }

    void AgentStream::startStream(const std::string& pipeline_str)
    {
        stream_thread_ = std::thread([this, pipeline_str]() {
            RCLCPP_INFO(node_->get_logger(), "Launching pipeline: %s", pipeline_str.c_str());

            GError* error = nullptr;
            pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);

            if (error)
            {
                RCLCPP_ERROR(node_->get_logger(), "Failed to parse pipeline: %s", error->message);
                g_error_free(error);
                return;
            }

            // Retrieve crop elements
            croppers_.clear();
            for (size_t i = 0; i < tracking_stream_infos_.size(); ++i)
            {
                std::string name = "crop_" + std::to_string(i);
                GstElement* cropper = gst_bin_get_by_name(GST_BIN(pipeline_), name.c_str());
                if (cropper)
                {
                    croppers_.push_back(cropper);
                }
                else
                {
                    croppers_.push_back(nullptr);
                    RCLCPP_ERROR(node_->get_logger(), "Could not find element named '%s'", name.c_str());
                }
            }

            // Start pipeline
            running_ = true;
            RCLCPP_INFO(node_->get_logger(), "Pipeline started");
            gst_element_set_state(pipeline_, GST_STATE_PLAYING);
            });
    }

    void AgentStream::stopStream()
    {
        RCLCPP_INFO(node_->get_logger(), "Stopping pipeline...");

        if (stream_thread_.joinable())
        {
            stream_thread_.join();
        }

        running_ = false;

        // Clear crops references
        for (auto* cropper : croppers_)
        {
            if (cropper)
            {
                gst_object_unref(cropper);
            }
        }
        croppers_.clear();

        if (pipeline_)
        {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
        }
        RCLCPP_INFO(node_->get_logger(), "Pipeline stopped");
    }

} // namespace flychams::agent