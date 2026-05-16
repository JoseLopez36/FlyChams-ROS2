#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_coordinator/assignment/agent_assignment.hpp"

// Base node include
#include "flychams_common/base/base_status_discoverer_node.hpp"

using namespace flychams::common;

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
class AgentAssignmentNode : public BaseStatusDiscovererNode
{
public: // Constructor/Destructor
    AgentAssignmentNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseStatusDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize agent assignment system
        agent_assignment_ = std::make_shared<AgentAssignment>(sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Agent assignment created");
    }

    void onDiscoveryShutdown() override
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