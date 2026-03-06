#include "flychams_agent/stream/agent_stream.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void AgentStream::onInit()
    {
        // Initialize GStreamer
        gst_init(nullptr, nullptr);

        // Get parameters
        host_ = RosUtils::getParameterOr<std::string>(node_, "stream_host", "127.0.0.1");
        source_port_ = RosUtils::getParameterOr<int>(node_, "stream_source_port", 5000);
        yolo_port_ = RosUtils::getParameterOr<int>(node_, "stream_yolo_port", 6000);
        interface_port_ = RosUtils::getParameterOr<int>(node_, "stream_interface_port", 7000);
        gpu_type_ = RosUtils::getParameterOr<std::string>(node_, "gpu_type", "auto");

        if (gpu_type_ == "auto")
        {
            gpu_type_ = detectGpuType();
            RCLCPP_INFO(node_->get_logger(), "Auto-detected GPU type: %s", gpu_type_.c_str());
        }

        // Create control subscriber
        // control_sub_ = node_->create_subscription<StringMsg>(
        //     "agent_stream/control", 10,
        //     std::bind(&AgentStream::controlCallback, this, std::placeholders::_1),
        //     sub_options_with_module_cb_group_);

        // Start pipeline
        startStream();
    }

    void AgentStream::onShutdown()
    {
        stopStream();
        control_sub_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void AgentStream::controlCallback(const core::StringMsg::SharedPtr msg)
    {

    }

    // ════════════════════════════════════════════════════════════════════════════
    // STREAM CONFIGURATION
    // ════════════════════════════════════════════════════════════════════════════

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

        RCLCPP_WARN(node_->get_logger(), "No hardware acceleration found. Defaulting to AMD (VAAPI) pipeline");
        return "amd";
    }

    std::string AgentStream::createPipeline()
    {
        if (gpu_type_ == "nvidia")
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
        std::string pipeline_str =
            "udpsrc port=" + std::to_string(source_port_) + " buffer-size=5242880 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265, payload=(int)96\" ! "
            "rtph265depay ! h265parse ! nvh265dec ! queue max-size-buffers=1 ! tee name=t ";

        // YOLO branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videoscale ! video/x-raw,width=640,height=640 ! nvh264enc bitrate=1000 rc-mode=cbr ! h264parse config-interval=-1 ! tcpserversink host=" + host_ + " port=" + std::to_string(yolo_port_) + " sync=false ";

        // Full res branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videoscale ! video/x-raw,width=1280,height=720 ! nvh264enc bitrate=3000 rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + host_ + " port=" + std::to_string(interface_port_) + " sync=false ";

        // Crop branches
        for (int i = 1; i <= 4; ++i)
        {
            std::string crop_name = "crop_" + std::to_string(i);
            int crop_port = interface_port_ + i;
            pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videocrop name=" + crop_name + " ! videoscale ! video/x-raw,width=1280,height=720 ! nvh264enc bitrate=3000 rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + host_ + " port=" + std::to_string(crop_port) + " sync=false ";
        }

        return pipeline_str;
    }

    std::string AgentStream::createAmdPipeline()
    {
        std::string pipeline_str =
            "udpsrc port=" + std::to_string(source_port_) + " buffer-size=5242880 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265, payload=(int)96\" ! "
            "rtph265depay ! h265parse ! vah265dec ! queue max-size-buffers=1 ! tee name=t ";

        // YOLO branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! vapostproc ! video/x-raw,width=640,height=640 ! vah264enc bitrate=1000 ! h264parse config-interval=-1 ! tcpserversink host=" + host_ + " port=" + std::to_string(yolo_port_) + " sync=false ";

        // Full res branch
        pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! vapostproc ! video/x-raw,width=1280,height=720 ! vah264enc bitrate=3000 rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + host_ + " port=" + std::to_string(interface_port_) + " sync=false ";

        // Crop branches
        for (int i = 1; i <= 4; ++i)
        {
            std::string crop_name = "crop_" + std::to_string(i);
            int crop_port = interface_port_ + i;
            pipeline_str += "t. ! queue leaky=downstream max-size-buffers=10 ! videocrop name=" + crop_name + " ! vapostproc ! video/x-raw,width=1280,height=720 ! vah264enc bitrate=3000 rc-mode=cbr ! h264parse config-interval=-1 ! mpegtsmux ! udpsink host=" + host_ + " port=" + std::to_string(crop_port) + " sync=false ";
        }

        return pipeline_str;
    }

    void AgentStream::startStream()
    {
        std::string pipeline_str = createPipeline();
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
        crops_.clear();
        for (int i = 1; i <= 4; ++i)
        {
            std::string name = "crop_" + std::to_string(i);
            GstElement* cropper = gst_bin_get_by_name(GST_BIN(pipeline_), name.c_str());
            if (cropper)
            {
                crops_[i] = cropper;
            }
            else
            {
                RCLCPP_ERROR(node_->get_logger(), "Could not find element named '%s'", name.c_str());
            }
        }

        // Start pipeline
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
        running_ = true;
        RCLCPP_INFO(node_->get_logger(), "Pipeline started");
    }

    void AgentStream::stopStream()
    {
        RCLCPP_INFO(node_->get_logger(), "Stopping pipeline...");
        running_ = false;
        if (pipeline_)
        {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
        }
        RCLCPP_INFO(node_->get_logger(), "Pipeline stopped");
    }

} // namespace flychams::agent