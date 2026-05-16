#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_simulation/bridge/agent_bridge.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

using namespace flychams::simulation;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent simulation bridge node for bridging agent observation
 * setpoints to AirSim wrapper commands
 *
 * @details
 * This class implements the agent simulation bridge node that listens
 * to observation_setpoints per agent and publishes CameraFovCmd
 * and GimbalAngleCmd messages to the AirSim wrapper.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-05-14
 * ════════════════════════════════════════════════════════════════
 */
class AgentBridgeNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    AgentBridgeNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize agent bridges map
        agent_bridges_.clear();
    }

    void onDiscoveryShutdown() override
    {
        // Destroy agent bridges
        agent_bridges_.clear();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Create agent simulation bridge
        auto bridge = std::make_shared<AgentBridge>(agent_id, sharedFromThis());
        agent_bridges_.insert(std::make_pair(agent_id, bridge));
        
        RCLCPP_INFO(node_->get_logger(), "Agent simulation bridge created for agent: %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Destroy agent simulation bridge
        agent_bridges_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Agent simulation bridge destroyed for agent: %s", agent_id.c_str());
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
    // Agent bridges
    std::unordered_map<ID, AgentBridge::SharedPtr> agent_bridges_;
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
    auto node = std::make_shared<AgentBridgeNode>("agent_bridge_node", options);
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
