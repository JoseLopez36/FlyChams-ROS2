#include "rclcpp/rclcpp.hpp"

// Simulation bridge includes
#include "flychams_simulation/agent/agent_simulation_bridge.hpp"

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;
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
class AgentSimulationBridgeNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    AgentSimulationBridgeNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Initialize agent bridges map
        agent_bridges_.clear();
    }

    void onShutdown() override
    {
        // Destroy agent bridges
        agent_bridges_.clear();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Create callback group for agent bridge
        auto bridge_cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

        // Create agent simulation bridge
        auto bridge = std::make_shared<AgentSimulationBridge>(agent_id, node_, settings_tools_, topic_tools_, transform_tools_, bridge_cb_group);
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
    // Agent simulation bridges
    std::unordered_map<ID, AgentSimulationBridge::SharedPtr> agent_bridges_;
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
    auto node = std::make_shared<AgentSimulationBridgeNode>("agent_simulation_bridge_node", options);
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
