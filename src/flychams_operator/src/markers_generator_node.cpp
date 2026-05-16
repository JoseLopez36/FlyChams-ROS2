#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_operator/markers/agent_markers.hpp"
#include "flychams_operator/markers/target_markers.hpp"
#include "flychams_operator/markers/cluster_markers.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Markers node: publishes visualization MarkerArrays for
 * agents, targets and clusters for the Foxglove operator interface
 *
 * @details
 * Dynamically discovers agents, targets and clusters via the
 * registration topic. Creates one AgentMarkers, TargetMarkers
 * and ClusterMarkers module per discovered element and destroys
 * them when the element is removed.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-05-14
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
        // Clear all marker generators
        agent_markers_.clear();
        target_markers_.clear();
        cluster_markers_.clear();
    }

    void onDiscoveryShutdown() override
    {
        // Clear all marker generators
        agent_markers_.clear();
        target_markers_.clear();
        cluster_markers_.clear();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Initialize agent marker generator
        auto agent_marker = std::make_shared<AgentMarkers>(agent_id, sharedFromThis());
        agent_markers_.insert(std::make_pair(agent_id, agent_marker));
        
        RCLCPP_INFO(node_->get_logger(), "Markers node: agent markers created for %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Destroy agent marker generator
        agent_markers_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: agent markers destroyed for %s", agent_id.c_str());
    }

    void onAddTarget(const ID& target_id) override
    {
        // Initialize target marker generator
        auto target_marker = std::make_shared<TargetMarkers>(target_id, sharedFromThis());
        target_markers_.insert(std::make_pair(target_id, target_marker));
        
        RCLCPP_INFO(node_->get_logger(), "Markers node: target markers created for %s", target_id.c_str());
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Destroy target marker generator
        target_markers_.erase(target_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: target markers destroyed for %s", target_id.c_str());
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Initialize cluster marker generator
        auto cluster_marker = std::make_shared<ClusterMarkers>(cluster_id, sharedFromThis());
        cluster_markers_.insert(std::make_pair(cluster_id, cluster_marker));
        
        RCLCPP_INFO(node_->get_logger(), "Markers node: cluster markers created for %s", cluster_id.c_str());
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Destroy cluster marker generator
        cluster_markers_.erase(cluster_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: cluster markers destroyed for %s", cluster_id.c_str());
    }

private: // Components
    // Marker generators
    std::unordered_map<ID, AgentMarkers::SharedPtr> agent_markers_;
    std::unordered_map<ID, TargetMarkers::SharedPtr> target_markers_;
    std::unordered_map<ID, ClusterMarkers::SharedPtr> cluster_markers_;
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
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    // Spin node
    executor.spin();
    // Shutdown
    rclcpp::shutdown();
    return 0;
}