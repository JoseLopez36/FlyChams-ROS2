#include "rclcpp/rclcpp.hpp"

// Fleet manager include
#include "flychams_coordinator/fleet/fleet_status.hpp"

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;
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
class FleetStatusNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    FleetStatusNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        fleet_manager_ = std::make_shared<FleetStatus>(node_, settings_tools_, topic_tools_, transform_tools_, discovery_cb_group_);
        RCLCPP_INFO(node_->get_logger(), "Fleet manager created");
    }

    void onShutdown() override
    {
        fleet_manager_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        fleet_manager_->addAgent(agent_id);
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        fleet_manager_->removeAgent(agent_id);
    }

    void onAddTarget(const ID& target_id) override {}
    void onRemoveTarget(const ID& target_id) override {}
    void onAddCluster(const ID& cluster_id) override {}
    void onRemoveCluster(const ID& cluster_id) override {}

private: // Components
    FleetStatus::SharedPtr fleet_manager_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(true);
    auto node = std::make_shared<FleetStatusNode>("fleet_status_node", options);
    node->init();
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}
