#include "rclcpp/rclcpp.hpp"

// Control includes
#include "flychams_agent/camera/camera_frames.hpp"

// Core includes
#include "flychams_core/base/base_node_with_tools.hpp"

using namespace flychams::core;
using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Control node for managing the frames of the agent's
 * cameras (gimbal/camera)
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-12-09
 * ════════════════════════════════════════════════════════════════
 */
class CameraFramesNode : public BaseNodeWithTools
{
public: // Constructor/Destructor
    CameraFramesNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNodeWithTools(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Create camera frames
        camera_frames_ = std::make_shared<CameraFrames>(agent_id_, node_, settings_tools_, topic_tools_, transform_tools_, node_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Camera frames created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
    {
        // Destroy camera frames
        camera_frames_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Camera frames
    CameraFrames::SharedPtr camera_frames_;
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
    auto node = std::make_shared<CameraFramesNode>("camera_frames_node", options);
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