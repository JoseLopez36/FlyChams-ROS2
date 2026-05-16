#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_agent/analysis/agent_analysis.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

using namespace flychams::common;

using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent analysis node for analyzing agent data
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-28
 * ════════════════════════════════════════════════════════════════
 */
class AgentAnalysisNode : public BaseNode
{
public: // Constructor/Destructor
    AgentAnalysisNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNode(node_name, options)
    {
        // Nothing to do
    }

    void onNodeInit() override
    {
        // Get agent ID
        agent_id_ = getParameter<std::string>("agent_id");

        // Initialize agent analysis system
        agent_analysis_ = std::make_shared<AgentAnalysis>(agent_id_, sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Agent analysis created for agent: %s", agent_id_.c_str());
    }

    void onNodeShutdown() override
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
