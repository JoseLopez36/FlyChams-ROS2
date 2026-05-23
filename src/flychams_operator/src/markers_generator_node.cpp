#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_operator/markers/markers_generator.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Markers generator node: publishes a single SceneUpdate
 *        message containing markers for all agents, targets and
 *        clusters
 *
 * @details
 * Dynamically discovers agents, targets and clusters via the
 * registration topic and delegates element management to the
 * MarkersGenerator module. All entities are batched into one
 * SceneUpdate publish per timer tick.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-05-23
 * ════════════════════════════════════════════════════════════════
 */
class MarkersGeneratorNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    MarkersGeneratorNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Create the markers generator module
        generator_ = std::make_shared<MarkersGenerator>(sharedFromThis());
    }

    void onDiscoveryShutdown() override
    {
        generator_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        generator_->addAgent(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: agent markers created for %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        generator_->removeAgent(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: agent markers destroyed for %s", agent_id.c_str());
    }

    void onAddTarget(const ID& target_id) override
    {
        generator_->addTarget(target_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: target markers created for %s", target_id.c_str());
    }

    void onRemoveTarget(const ID& target_id) override
    {
        generator_->removeTarget(target_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: target markers destroyed for %s", target_id.c_str());
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
    MarkersGenerator::SharedPtr generator_;
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
    auto node = std::make_shared<MarkersGeneratorNode>("markers_generator_node", options);
    node->init();
    // Create executor and add node
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    // Spin node
    executor.spin();
    // Shutdown
    rclcpp::shutdown();
    return 0;
}