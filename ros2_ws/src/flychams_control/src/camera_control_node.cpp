#include "rclcpp/rclcpp.hpp"

// Control includes
#include "flychams_control/camera/camera_control.hpp"

// Core includes
#include "flychams_core/base/base_discoverer_node.hpp"
#include "flychams_core/utils/ros_utils.hpp"

using namespace flychams::core;
using namespace flychams::control;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Control node for controlling the agent's cameras (gimbal/
 * camera)
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-02-28
 * ════════════════════════════════════════════════════════════════
 */
class CameraControlNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    CameraControlNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Create camera control
        camera_control_ = std::make_shared<CameraControl>(agent_id_, node_, config_tools_, topic_tools_, transform_tools_, nullptr);

        RCLCPP_INFO(node_->get_logger(), "Camera Control created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
    {
        // Destroy camera control
        camera_control_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Agents are not handled by this node (uses agent_id from parameter)
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Agents are not handled by this node (uses agent_id from parameter)
    }

    void onAddTarget(const ID& target_id) override
    {
        // Targets are not handled by this node
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Targets are not handled by this node
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Clusters are not handled by this node
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Clusters are not handled by this node
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Camera control
    CameraControl::SharedPtr camera_control_;
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
    auto node = std::make_shared<CameraControlNode>("camera_control_node", options);
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