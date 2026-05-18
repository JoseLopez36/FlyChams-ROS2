#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_agent/state/agent_state.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

using namespace flychams::common;

using namespace flychams::agent;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Control node for managing the state of the drones
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-31
 * ════════════════════════════════════════════════════════════════
 */
class AgentStateNode : public BaseStatusNode
{
public: // Constructor/Destructor
    AgentStateNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseStatusNode(node_name, options)
    {
        // Nothing to do
    }

    void onStatusInit() override
    {
        // Get agent ID
        agent_id_ = getParameter<std::string>("agent_id");

        // Initialize TF
        node_->initTf();
        
        // Create drone state
        drone_state_ = std::make_shared<DroneState>(agent_id_, sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Drone state created for agent: %s", agent_id_.c_str());
    }

    void onStatusShutdown() override
    {
        // Destroy drone state
        drone_state_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Drone state
    DroneState::SharedPtr drone_state_;
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
    auto node = std::make_shared<AgentStateNode>("agent_state_node", options);
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