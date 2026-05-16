#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_coordinator/state/fleet_state.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Fleet status node - aggregates agent statuses and
 * manages the mission state machine.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-05-14
 * ════════════════════════════════════════════════════════════════
 */
class FleetStateNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    FleetStateNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize fleet manager
        fleet_state_ = std::make_shared<FleetState>(sharedFromThis());
        
        RCLCPP_INFO(node_->get_logger(), "Fleet manager created");
    }

    void onDiscoveryShutdown() override
    {
        fleet_state_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        fleet_state_->addAgent(agent_id);
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        fleet_state_->removeAgent(agent_id);
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
    // Fleet status manager
    FleetState::SharedPtr fleet_state_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(true);
    auto node = std::make_shared<FleetStateNode>("fleet_status_node", options);
    node->init();
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}
