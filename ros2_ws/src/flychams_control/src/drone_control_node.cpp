#include "rclcpp/rclcpp.hpp"

// Control includes
#include "flychams_control/drone/drone_control.hpp"

// Core includes
#include "flychams_core/base/base_discoverer_node.hpp"
#include "flychams_core/utils/ros_utils.hpp"

using namespace flychams::core;
using namespace flychams::control;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Control node for controlling each drone registered
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-31
 * ════════════════════════════════════════════════════════════════
 */
class DroneControlNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    DroneControlNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Create drone control
        drone_control_ = std::make_shared<DroneControl>(agent_id_, node_, config_tools_, topic_tools_, transform_tools_, nullptr);

        RCLCPP_INFO(node_->get_logger(), "Drone Control created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
    {
        // Destroy drone control
        drone_control_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Drone control
    DroneControl::SharedPtr drone_control_;
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
    auto node = std::make_shared<DroneControlNode>("drone_control_node", options);
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