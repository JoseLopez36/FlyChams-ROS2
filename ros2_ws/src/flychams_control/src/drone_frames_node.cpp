#include "rclcpp/rclcpp.hpp"

// Control includes
#include "flychams_control/drone/drone_frames.hpp"

// Core includes
#include "flychams_core/base/base_node_with_tools.hpp"

using namespace flychams::core;
using namespace flychams::control;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Control node for managing the frames of the drones
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-12-9
 * ════════════════════════════════════════════════════════════════
 */
class DroneFramesNode : public BaseNodeWithTools
{
public: // Constructor/Destructor
    DroneFramesNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNodeWithTools(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Create drone frames
        drone_frames_ = std::make_shared<DroneFrames>(agent_id_, node_, config_tools_, topic_tools_, transform_tools_, node_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Drone frames created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
    {
        // Destroy drone frames
        drone_frames_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Drone frames
    DroneFrames::SharedPtr drone_frames_;
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
    auto node = std::make_shared<DroneFramesNode>("drone_frames_node", options);
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