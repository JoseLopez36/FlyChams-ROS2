#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_simulation/target/target_state.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

using namespace flychams::simulation;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Target node for state management of the targets
 *
 * @details
 * This class implements the target node for state management of the
 * targets. It is a debugging node that allows to know the state of the
 * targets. In a real scenario, this node would be replaced by a node
 * that uses images to detect the state of the targets.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-27
 * ════════════════════════════════════════════════════════════════
 */
class TargetStateNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    TargetStateNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize target states
        target_state_.clear();
    }

    void onDiscoveryShutdown() override
    {
        // Destroy target states
        target_state_.clear();
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
        // Create target controllers
        auto target_state = std::make_shared<TargetState>(target_id, sharedFromThis());
        target_state_.insert(std::make_pair(target_id, target_state));

        RCLCPP_INFO(node_->get_logger(), "Target state created for target %s", target_id.c_str());
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Destroy target controllers
        target_state_.erase(target_id);
        RCLCPP_INFO(node_->get_logger(), "Target state destroyed for target %s", target_id.c_str());
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
    // Target states
    std::unordered_map<ID, TargetState::SharedPtr> target_state_;
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
    auto node = std::make_shared<TargetStateNode>("target_state_node", options);
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