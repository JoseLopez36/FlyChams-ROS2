#include "rclcpp/rclcpp.hpp"

// Coordination includes
#include "flychams_agent/positioning/agent_positioning.hpp"

// Core includes
#include "flychams_common/base/base_node_with_tools.hpp"

using namespace flychams::core;
using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent positioning node for positioning agents in the
 * simulation
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-02-28
 * ════════════════════════════════════════════════════════════════
 */
class AgentPositioningNode : public BaseNodeWithTools
{
public: // Constructor/Destructor
    AgentPositioningNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNodeWithTools(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Create agent positioning
        agent_positioning_ = std::make_shared<AgentPositioning>(agent_id_, node_, settings_tools_, topic_tools_, transform_tools_, node_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Agent positioning created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
    {
        // Destroy agent positioning
        agent_positioning_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Agent positioning
    AgentPositioning::SharedPtr agent_positioning_;
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
    auto node = std::make_shared<AgentPositioningNode>("agent_positioning_node", options);
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