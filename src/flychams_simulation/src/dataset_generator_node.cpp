#include "rclcpp/rclcpp.hpp"

// Standard includes
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"
#include "flychams_common/utils/vision_utils.hpp"

using namespace flychams::core;

namespace fs = std::filesystem;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Dataset generator node for generating the dataset for the
 * simulation.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-02-03
 * ════════════════════════════════════════════════════════════════
 */
class DatasetGeneratorNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    DatasetGeneratorNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get parameters from parameter server
        sampling_interval_ = RosUtils::getParameterOr<float>(node_, "sampling_interval", 0.1f);
        output_directory_ = RosUtils::getParameterOr<std::string>(node_, "output_directory", "dataset");
        udp_port_ = RosUtils::getParameterOr<int>(node_, "udp_port", 5000);
        agent_id_ = RosUtils::getParameterOr<std::string>(node_, "agent_id", "AGENT00");
        camera_id_ = RosUtils::getParameterOr<std::string>(node_, "camera_id", "MULTICAMERA00");

        // Create output directories
        try {
            // Use FLYCHAMS_ROS2_PATH env var as base directory
            const char* env_path = std::getenv("FLYCHAMS_ROS2_PATH");
            if (!env_path) {
                RCLCPP_ERROR(this->get_logger(), "FLYCHAMS_ROS2_PATH environment variable not set.");
                rclcpp::shutdown();
                throw std::runtime_error("FLYCHAMS_ROS2_PATH not set");
            }
            fs::path base_path = fs::path(env_path) / output_directory_;
            fs::create_directories(base_path / "images");
            fs::create_directories(base_path / "json");
            
            // Initialize CSV file
            csv_file_.open(base_path / "dataset.csv", std::ios::out);
            if (csv_file_.is_open()) {
                csv_file_ << "index,timestamp,agent_x,agent_y,agent_z,cam_x,cam_y,cam_z,cam_qx,cam_qy,cam_qz,cam_qw\n";
            } else {
                RCLCPP_ERROR(this->get_logger(), "Could not open dataset.csv for writing at %s", output_directory_.c_str());
            }
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Error creating directories: %s", e.what());
            rclcpp::shutdown();
            throw std::runtime_error("Error creating directories");
        }

        // Open UDP stream
        std::string url = "udp://127.0.0.1:" + std::to_string(udp_port_) + "?fifo_size=5000000&overrun_nonfatal=1";
        video_cap_.open(url, cv::CAP_FFMPEG);
        if (!video_cap_.isOpened()) 
        {
            RCLCPP_ERROR(this->get_logger(), "Could not open UDP stream: %s", url.c_str());
            rclcpp::shutdown();
            throw std::runtime_error("Could not open UDP stream");
        }

        // Set update timer
        update_timer_ = rclcpp::create_timer(node_, 
            node_->get_clock(),
            std::chrono::duration<float>(sampling_interval_), 
            std::bind(&DatasetGeneratorNode::update, this), 
            discovery_cb_group_);

        RCLCPP_INFO(this->get_logger(), "DatasetGeneratorNode initialized with sampling interval: %.2fs", sampling_interval_);
    }

    void onShutdown() override
    {
        // Close CSV file
        if (csv_file_.is_open()) {
            csv_file_.close();
        }
        // Release video capture
        if (video_cap_.isOpened()) {
            video_cap_.release();
        }
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        if (agent_id == agent_id_) 
        {
            RCLCPP_INFO(this->get_logger(), "Monitoring agent: %s", agent_id.c_str());
            // Create agent position subscriber
            agent_pos_sub_ = topic_tools_->createAgentGlobalPositionSubscriber(agent_id,
                [this](const PointStampedMsg::SharedPtr msg)
                {
                    this->agent_position_ = *msg;
                    this->has_agent_position_ = true;
                }, sub_options_with_discovery_cb_group_);
        }
    }

    void onAddTarget(const ID& target_id) override
    {
        RCLCPP_INFO(this->get_logger(), "Adding target to monitor: %s", target_id.c_str());
		// Create target true position subscriber
		target_subs_[target_id] = topic_tools_->createTargetTruePositionSubscriber(target_id,
			[this, target_id](const PointStampedMsg::SharedPtr msg)
			{
				this->target_positions_[target_id] = *msg;
			}, sub_options_with_discovery_cb_group_);
    }

    void update()
    {
        // Capture frame from video stream
        cv::Mat frame;
        if (!video_cap_.read(frame)) 
        {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Empty frame captured from UDP stream");
            return;
        }

        if (!has_agent_position_) 
        {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Waiting for agent position...");
            return;
        }

        // Lookup camera pose
        std::string world_frame = transform_tools_->getGlobalFrame();
        std::string camera_frame = transform_tools_->getCameraOpticalFrame(agent_id_, camera_id_);
        TransformMsg cam_transform = transform_tools_->getTransform(world_frame, camera_frame);

        // Save image and metadata
        fs::path env_path = fs::path(std::getenv("FLYCHAMS_ROS2_PATH"));
        fs::path output_path = env_path / output_directory_;
        std::string index_str = std::to_string(dataset_index_);
        
        // Save image
        std::string img_path = (output_path / "images" / (index_str + ".png")).string();
        cv::imwrite(img_path, frame);

        // Save metadata to CSV
        if (csv_file_.is_open()) {
            csv_file_ << dataset_index_ << ","
                      << this->now().seconds() << ","
                      << agent_position_.point.x << "," << agent_position_.point.y << "," << agent_position_.point.z << ","
                      << cam_transform.translation.x << "," << cam_transform.translation.y << "," << cam_transform.translation.z << ","
                      << cam_transform.rotation.x << "," << cam_transform.rotation.y << "," << cam_transform.rotation.z << "," << cam_transform.rotation.w << "\n";
            csv_file_.flush();
        }

        // Save detailed metadata to JSON
        std::string json_path = (output_path / "json" / (index_str + ".json")).string();
        std::ofstream json_file(json_path);
        if (json_file.is_open()) {
            json_file << "{\n";
            json_file << "  \"index\": " << dataset_index_ << ",\n";
            json_file << "  \"timestamp\": " << this->now().seconds() << ",\n";
            json_file << "  \"agent_position\": {\"x\": " << agent_position_.point.x << ", \"y\": " << agent_position_.point.y << ", \"z\": " << agent_position_.point.z << "},\n";
            json_file << "  \"camera_pose\": {\n";
            json_file << "    \"position\": {\"x\": " << cam_transform.translation.x << ", \"y\": " << cam_transform.translation.y << ", \"z\": " << cam_transform.translation.z << "},\n";
            json_file << "    \"orientation\": {\"x\": " << cam_transform.rotation.x << ", \"y\": " << cam_transform.rotation.y << ", \"z\": " << cam_transform.rotation.z << ", \"w\": " << cam_transform.rotation.w << "}\n";
            json_file << "  },\n";
            json_file << "  \"targets\": {\n";
            for (auto it = target_positions_.begin(); it != target_positions_.end(); ++it) 
            {
                json_file << "    \"" << it->first << "\": {\"x\": " << it->second.point.x << ", \"y\": " << it->second.point.y << ", \"z\": " << it->second.point.z << "}"
                          << (std::next(it) == target_positions_.end() ? "" : ",") << "\n";
            }
            json_file << "  }\n";
            json_file << "}";
            json_file.close();
        }

        dataset_index_++;
    }

private: // Data
    // Parameters
    float sampling_interval_;
    std::string output_directory_;
    int udp_port_;
    std::string agent_id_;
    std::string camera_id_;

    // ROS components
    TimerPtr update_timer_;
    SubscriberPtr<PointStampedMsg> agent_pos_sub_;
    std::map<ID, SubscriberPtr<PointStampedMsg>> target_subs_;

    // OpenCV components
    cv::VideoCapture video_cap_;

    // Dataset state
    uint64_t dataset_index_ = 0;
    std::ofstream csv_file_;

    // Data
    PointStampedMsg agent_position_;
    bool has_agent_position_ = false;
    std::map<ID, PointStampedMsg> target_positions_;
};

int main(int argc, char** argv)
{
    // Initialize ROS
    rclcpp::init(argc, argv);
    // Create node options
    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(true);
    // Create and initialize node
    auto node = std::make_shared<DatasetGeneratorNode>("dataset_generator_node", options);
    node->init();
    // Create executor and add node
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    // Spin node
    executor.spin();
    // Shutdown
    rclcpp::shutdown();
    return 0;
}