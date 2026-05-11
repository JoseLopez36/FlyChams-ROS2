#include "rclcpp/rclcpp.hpp"

// Coordination includes
#include "flychams_coordinator/assignment/agent_assignment.hpp"

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::coordinator;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent assignment node for assigning clusters to agents
 * in the simulation
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-02-28
 * ════════════════════════════════════════════════════════════════
 */
class AgentAssignmentNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    AgentAssignmentNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Use callback group from discovery node (to avoid race conditions)
        // Initialize agent assignment system
        agent_assignment_ = std::make_shared<AgentAssignment>(node_, settings_tools_, topic_tools_, transform_tools_, discovery_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Agent assignment created");
    }

    void onShutdown() override
    {
        // Destroy agent assignment system
        agent_assignment_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Add agent to assignment manager
        agent_assignment_->addAgent(agent_id);
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Remove agent from assignment manager
        agent_assignment_->removeAgent(agent_id);
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
        // Add cluster to assignment manager
        agent_assignment_->addCluster(cluster_id);
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Remove cluster from assignment manager
        agent_assignment_->removeCluster(cluster_id);
    }

private: // Components
    // Agent assignment system
    AgentAssignment::SharedPtr agent_assignment_;
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
    auto node = std::make_shared<AgentAssignmentNode>("agent_assignment_node", options);
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