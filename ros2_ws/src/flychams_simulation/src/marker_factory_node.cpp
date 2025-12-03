#include "rclcpp/rclcpp.hpp"

// Simulation includes
#include "flychams_simulation/marker/marker_factory.hpp"

// Core includes
#include "flychams_core/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::simulation;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Marker factory node for the FlyingChameleons system
 *
 * @details
 * This class implements the marker factory node for the FlyingChameleons
 * system. It uses the discoverer node to discover the different elements
 * and then creates a marker factory for each element discovered.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-01
 * ════════════════════════════════════════════════════════════════
 */
class MarkerFactoryNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    MarkerFactoryNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Use callback group from discovery node (to avoid race conditions)
        // Initialize marker factory
        marker_factory_ = std::make_shared<MarkerFactory>(node_, config_tools_, topic_tools_, transform_tools_, discovery_cb_group_);
    }

    void onShutdown() override
    {
        // Destroy marker factory
        marker_factory_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Add agent to marker factory
        marker_factory_->addAgent(agent_id);
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Remove agent from marker factory
        marker_factory_->removeAgent(agent_id);
    }

    void onAddTarget(const ID& target_id) override
    {
        // Add target to marker factory
        marker_factory_->addTarget(target_id);
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Remove target from marker factory
        marker_factory_->removeTarget(target_id);
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Add cluster to marker factory
        marker_factory_->addCluster(cluster_id);
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Remove cluster from marker factory
        marker_factory_->removeCluster(cluster_id);
    }

private: // Components
    // Marker factory
    MarkerFactory::SharedPtr marker_factory_;
};

int main(int argc, char** argv)
{
    // Initialize ROS
    rclcpp::init(argc, argv);
    // Create node options
    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(true);
    // Create node
    auto node = std::make_shared<MarkerFactoryNode>("marker_factory_node", options);
    node->init();
    // Spin node
    rclcpp::spin(node);
    // Shutdown
    rclcpp::shutdown();
    return 0;
}