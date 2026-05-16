#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_coordinator/analysis/cluster_analysis.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::coordinator;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Cluster analysis node for analyzing the clusters
 *
 * @details
 * This class implements the cluster analysis node for analyzing the
 * clusters in the simulation (e.g. minimum enclosing circle, centroid,
 * etc.). It uses the discoverer node to discover the different clusters
 * and then creates a cluster analysis for each cluster discovered.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-27
 * ════════════════════════════════════════════════════════════════
 */
class ClusterAnalysisNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    ClusterAnalysisNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize cluster analysis
        cluster_analysis_ = std::make_shared<ClusterAnalysis>(sharedFromThis());
        
        RCLCPP_INFO(node_->get_logger(), "Cluster analysis created");
    }

    void onDiscoveryShutdown() override
    {
        // Destroy cluster analysis
        cluster_analysis_.reset();
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
        // Add target to cluster analysis
        cluster_analysis_->addTarget(target_id);
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Remove target from cluster analysis
        cluster_analysis_->removeTarget(target_id);
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Add cluster to cluster analysis
        cluster_analysis_->addCluster(cluster_id);
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Remove cluster from cluster analysis
        cluster_analysis_->removeCluster(cluster_id);
    }

private: // Components
    // Cluster analysis
    ClusterAnalysis::SharedPtr cluster_analysis_;
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
    auto node = std::make_shared<ClusterAnalysisNode>("cluster_analysis_node", options);
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