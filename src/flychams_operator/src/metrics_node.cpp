#include "rclcpp/rclcpp.hpp"

// Metrics includes
#include "flychams_operator/metrics/agent_metrics.hpp"
#include "flychams_operator/metrics/target_metrics.hpp"
#include "flychams_operator/metrics/cluster_metrics.hpp"

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Metrics node: aggregates per-agent, per-target and
 * per-cluster metrics and publishes them for the operator interface
 *
 * @details
 * Dynamically discovers agents, targets and clusters via the
 * registration topic. Creates one AgentMetrics, TargetMetrics
 * and ClusterMetrics module per discovered element and destroys
 * them when the element is removed.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-05-14
 * ════════════════════════════════════════════════════════════════
 */
class MetricsNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    MetricsNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        agent_metrics_.clear();
        target_metrics_.clear();
        cluster_metrics_.clear();
    }

    void onShutdown() override
    {
        agent_metrics_.clear();
        target_metrics_.clear();
        cluster_metrics_.clear();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        auto cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        auto module = std::make_shared<flychams::operator_pkg::AgentMetrics>(agent_id, node_, settings_tools_, topic_tools_, transform_tools_, cb_group);
        agent_metrics_.insert(std::make_pair(agent_id, module));
        RCLCPP_INFO(node_->get_logger(), "Metrics node: agent metrics created for %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        agent_metrics_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Metrics node: agent metrics destroyed for %s", agent_id.c_str());
    }

    void onAddTarget(const ID& target_id) override
    {
        auto cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        auto module = std::make_shared<flychams::operator_pkg::TargetMetrics>(target_id, node_, settings_tools_, topic_tools_, transform_tools_, cb_group);
        target_metrics_.insert(std::make_pair(target_id, module));
        RCLCPP_INFO(node_->get_logger(), "Metrics node: target metrics created for %s", target_id.c_str());
    }

    void onRemoveTarget(const ID& target_id) override
    {
        target_metrics_.erase(target_id);
        RCLCPP_INFO(node_->get_logger(), "Metrics node: target metrics destroyed for %s", target_id.c_str());
    }

    void onAddCluster(const ID& cluster_id) override
    {
        auto cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        auto module = std::make_shared<flychams::operator_pkg::ClusterMetrics>(cluster_id, node_, settings_tools_, topic_tools_, transform_tools_, cb_group);
        cluster_metrics_.insert(std::make_pair(cluster_id, module));
        RCLCPP_INFO(node_->get_logger(), "Metrics node: cluster metrics created for %s", cluster_id.c_str());
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        cluster_metrics_.erase(cluster_id);
        RCLCPP_INFO(node_->get_logger(), "Metrics node: cluster metrics destroyed for %s", cluster_id.c_str());
    }

private: // Components
    std::unordered_map<ID, flychams::operator_pkg::AgentMetrics::SharedPtr> agent_metrics_;
    std::unordered_map<ID, flychams::operator_pkg::TargetMetrics::SharedPtr> target_metrics_;
    std::unordered_map<ID, flychams::operator_pkg::ClusterMetrics::SharedPtr> cluster_metrics_;
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
    auto node = std::make_shared<MetricsNode>("metrics_node", options);
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