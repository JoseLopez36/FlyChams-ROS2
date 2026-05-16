#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_coordinator/clustering/target_clustering.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::coordinator;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Clustering node for clustering the different targets
 * in the simulation
 *
 * @details
 * This class implements the clustering node for clustering the
 * different targets in the simulation. It uses the discoverer node to
 * discover the different targets and then creates a clustering for each
 * target discovered.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-01
 * ════════════════════════════════════════════════════════════════
 */
class TargetClusteringNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    TargetClusteringNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize target clustering
        target_clustering_ = std::make_shared<TargetClustering>(sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Target clustering created");
    }

    void onDiscoveryShutdown() override
    {
        // Destroy target clustering system
        target_clustering_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Agents are not handled by this node
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Agents are not handled by this node
    }

    void onAddTarget(const ID& target_id) override
    {
        // Add target to clustering
        target_clustering_->addTarget(target_id);
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Remove target from clustering
        target_clustering_->removeTarget(target_id);
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Add cluster to clustering
        target_clustering_->addCluster(cluster_id);
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Remove cluster from clustering
        target_clustering_->removeCluster(cluster_id);
    }

private: // Components
    // Target clustering system
    TargetClustering::SharedPtr target_clustering_;
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
    auto node = std::make_shared<TargetClusteringNode>("target_clustering_node", options);
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