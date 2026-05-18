#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_coordinator/state/mission_state.hpp"

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Mission status node - manages mission lifecycle and
 * publishes detailed mission state.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-05-16
 * ════════════════════════════════════════════════════════════════
 */
class MissionStateNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    MissionStateNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onDiscoveryInit() override
    {
        // Initialize mission manager
        mission_state_ = std::make_shared<MissionState>(sharedFromThis());
        
        RCLCPP_INFO(node_->get_logger(), "Mission manager created");
    }

    void onDiscoveryShutdown() override
    {
        mission_state_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        mission_state_->addAgent(agent_id);
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        mission_state_->removeAgent(agent_id);
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
    // Mission status manager
    MissionState::SharedPtr mission_state_;
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
    auto node = std::make_shared<MissionStateNode>("mission_state_node", options);
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