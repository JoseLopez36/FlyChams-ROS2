#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_agent/stream/agent_stream.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

using namespace flychams::common;

using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent stream node for handling video streaming and cropping
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-03-06
 * ════════════════════════════════════════════════════════════════
 */
class AgentStreamNode : public BaseNode
{
public: // Constructor/Destructor
    AgentStreamNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNode(node_name, options)
    {
        // Nothing to do
    }

    void onNodeInit() override
    {
        // Get agent ID
        agent_id_ = getParameter<std::string>("agent_id");

        // Create agent stream module
        agent_stream_ = std::make_shared<AgentStream>(agent_id_, sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Agent stream created for agent: %s", agent_id_.c_str());
    }

    void onNodeShutdown() override
    {
        // Destroy agent stream module
        agent_stream_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Agent stream
    AgentStream::SharedPtr agent_stream_;
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
    auto node = std::make_shared<AgentStreamNode>("agent_stream_node", options);
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