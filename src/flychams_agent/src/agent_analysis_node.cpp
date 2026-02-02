#include "rclcpp/rclcpp.hpp"

// Coordination includes
#include "flychams_agent/analysis/agent_analysis.hpp"

// Core includes
#include "flychams_core/base/base_node_with_tools.hpp"

using namespace flychams::core;
using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent analysis node for analyzing agent data
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-28
 * ════════════════════════════════════════════════════════════════
 */
class AgentAnalysisNode : public BaseNodeWithTools
{
public: // Constructor/Destructor
    AgentAnalysisNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNodeWithTools(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Initialize agent analysis system
        agent_analysis_ = std::make_shared<AgentAnalysis>(agent_id_, node_, settings_tools_, topic_tools_, transform_tools_, node_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Agent analysis created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
    {
        // Destroy agent analysis system
        agent_analysis_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Agent analysis system
    AgentAnalysis::SharedPtr agent_analysis_;
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
    auto node = std::make_shared<AgentAnalysisNode>("agent_analysis_node", options);
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
