#include "rclcpp/rclcpp.hpp"

// Target includes
#include "flychams_simulation/target/target_control.hpp"

// Core includes
#include "flychams_core/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::simulation;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Target node for controlling the different targets in the
 * mission
 *
 * @details
 * This class implements the target node for controlling the different
 * targets in the mission. It uses the discoverer node to discover the
 * different targets and adds them to the target controller.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-16
 * ════════════════════════════════════════════════════════════════
 */
class TargetControlNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    TargetControlNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Use callback group from discovery node (to avoid race conditions)
        // Initialize target control
        target_control_ = std::make_shared<TargetControl>(node_, config_tools_, topic_tools_, transform_tools_, discovery_cb_group_);
    }

    void onShutdown() override
    {
        // Destroy target control
        target_control_.reset();
    }

private: // Element management
    void onAddTarget(const ID& target_id) override
    {
        // Add target to target control
        target_control_->addTarget(target_id);
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Remove target from target control
        target_control_->removeTarget(target_id);
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Add cluster to target control
        target_control_->addCluster(cluster_id);
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Remove cluster from target control
        target_control_->removeCluster(cluster_id);
    }

private: // Components
    // Target control
    TargetControl::SharedPtr target_control_;
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
    auto node = std::make_shared<TargetControlNode>("target_control_node", options);
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