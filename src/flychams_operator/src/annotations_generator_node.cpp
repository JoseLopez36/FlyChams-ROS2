#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_operator/annotations/agent_annotations.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Annotations node: publishes foxglove ImageAnnotations
 *        for each agent's observation units
 *
 * @details
 * Dynamically discovers agents via the registration topic.
 * Creates one AgentAnnotations module per discovered agent and
 * destroys it when the agent is removed.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-05-23
 * ════════════════════════════════════════════════════════════════
 */
class AnnotationsGeneratorNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    AnnotationsGeneratorNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize TF
        initTf();

        // Initialize agent annotations
        agent_annotations_.clear();
    }

    void onDiscoveryShutdown() override
    {
        agent_annotations_.clear();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        agent_annotations_.emplace(agent_id, std::make_shared<AgentAnnotations>(agent_id, sharedFromThis()));
        RCLCPP_INFO(node_->get_logger(), "Annotations node: agent annotations created for %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        agent_annotations_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Annotations node: agent annotations destroyed for %s", agent_id.c_str());
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
    std::unordered_map<ID, AgentAnnotations::SharedPtr> agent_annotations_;
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
    auto node = std::make_shared<AnnotationsGeneratorNode>("annotations_generator_node", options);
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