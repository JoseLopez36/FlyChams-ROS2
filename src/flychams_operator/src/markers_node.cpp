#include "rclcpp/rclcpp.hpp"

// Markers includes
#include "flychams_operator/markers/agent_markers.hpp"
#include "flychams_operator/markers/target_markers.hpp"
#include "flychams_operator/markers/cluster_markers.hpp"

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;

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
class MarkersNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    MarkersNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        agent_markers_.clear();
        target_markers_.clear();
        cluster_markers_.clear();
    }

    void onShutdown() override
    {
        agent_markers_.clear();
        target_markers_.clear();
        cluster_markers_.clear();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        auto cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        auto module = std::make_shared<flychams::operator_pkg::AgentMarkers>(agent_id, node_, settings_tools_, topic_tools_, transform_tools_, cb_group);
        agent_markers_.insert(std::make_pair(agent_id, module));
        RCLCPP_INFO(node_->get_logger(), "Markers node: agent markers created for %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        agent_markers_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: agent markers destroyed for %s", agent_id.c_str());
    }

    void onAddTarget(const ID& target_id) override
    {
        auto cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        auto module = std::make_shared<flychams::operator_pkg::TargetMarkers>(target_id, node_, settings_tools_, topic_tools_, transform_tools_, cb_group);
        target_markers_.insert(std::make_pair(target_id, module));
        RCLCPP_INFO(node_->get_logger(), "Markers node: target markers created for %s", target_id.c_str());
    }

    void onRemoveTarget(const ID& target_id) override
    {
        target_markers_.erase(target_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: target markers destroyed for %s", target_id.c_str());
    }

    void onAddCluster(const ID& cluster_id) override
    {
        auto cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        auto module = std::make_shared<flychams::operator_pkg::ClusterMarkers>(cluster_id, node_, settings_tools_, topic_tools_, transform_tools_, cb_group);
        cluster_markers_.insert(std::make_pair(cluster_id, module));
        RCLCPP_INFO(node_->get_logger(), "Markers node: cluster markers created for %s", cluster_id.c_str());
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        cluster_markers_.erase(cluster_id);
        RCLCPP_INFO(node_->get_logger(), "Markers node: cluster markers destroyed for %s", cluster_id.c_str());
    }

private: // Components
    std::unordered_map<ID, flychams::operator_pkg::AgentMarkers::SharedPtr> agent_markers_;
    std::unordered_map<ID, flychams::operator_pkg::TargetMarkers::SharedPtr> target_markers_;
    std::unordered_map<ID, flychams::operator_pkg::ClusterMarkers::SharedPtr> cluster_markers_;
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
    auto node = std::make_shared<MarkersNode>("markers_node", options);
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