#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_agent/tracking/agent_tracking.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

using namespace flychams::common;
using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent tracking node for tracking targets (clusters) in
 * the simulation
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-02-28
 * ════════════════════════════════════════════════════════════════
 */
class AgentTrackingNode : public BaseStatusNode
{
public: // Constructor/Destructor
    AgentTrackingNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseStatusNode(node_name, options)
    {
        // Nothing to do
    }

    void onStatusInit() override
    {
        // Get agent ID
        agent_id_ = getParameter<std::string>("agent_id");

        // Create agent tracking
        agent_tracking_ = std::make_shared<AgentTracking>(agent_id_, sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Agent tracking created for agent: %s", agent_id_.c_str());
    }

    void onStatusShutdown() override
    {
        // Destroy agent tracking
        agent_tracking_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Agent tracking
    AgentTracking::SharedPtr agent_tracking_;
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
    auto node = std::make_shared<AgentTrackingNode>("agent_tracking_node", options);
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