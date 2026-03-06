#include "rclcpp/rclcpp.hpp"

// Stream includes
#include "flychams_agent/stream/agent_stream.hpp"

// Core includes
#include "flychams_core/base/base_node_with_tools.hpp"

using namespace flychams::core;
using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Agent stream node for handling video streaming and cropping
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-03-06
 * ════════════════════════════════════════════════════════════════
 */
class AgentStreamNode : public BaseNodeWithTools
{
public: // Constructor/Destructor
    AgentStreamNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNodeWithTools(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Create agent stream module
        agent_stream_ = std::make_shared<AgentStream>(agent_id_, node_, settings_tools_, topic_tools_, transform_tools_, node_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Agent stream created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
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