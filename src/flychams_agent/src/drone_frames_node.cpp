#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_agent/drone/drone_frames.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

using namespace flychams::common;
using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Control node for managing the frames of the drones
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-12-9
 * ════════════════════════════════════════════════════════════════
 */
class DroneFramesNode : public BaseNode
{
public: // Constructor/Destructor
    DroneFramesNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNode(node_name, options)
    {
        // Nothing to do
    }

    void onNodeInit() override
    {
        // Get agent ID
        agent_id_ = getParameter<std::string>("agent_id");

        // Create drone frames
        drone_frames_ = std::make_shared<DroneFrames>(agent_id_, sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Drone frames created for agent: %s", agent_id_.c_str());
    }

    void onNodeShutdown() override
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