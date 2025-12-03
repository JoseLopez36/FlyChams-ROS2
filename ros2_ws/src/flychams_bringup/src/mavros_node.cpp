#include "rclcpp/rclcpp.hpp"

// Mavros includes
#include "flychams_bringup/mavros/mavros_manager.hpp"

// Core includes
#include "flychams_core/base/base_registrator_node.hpp"
#include "flychams_core/utils/ros_utils.hpp"

using namespace flychams::core;
using namespace flychams::bringup;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief MavROS node for managing connection to a single agent
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-12-01
 * ════════════════════════════════════════════════════════════════
 */
class MavrosNode : public BaseRegistratorNode
{
public: // Constructor/Destructor
    MavrosNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseRegistratorNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Get agent ID
        agent_id_ = RosUtils::getParameter<std::string>(node_, "agent_id");

        // Create mavros manager
        mavros_manager_ = std::make_shared<MavrosManager>(agent_id_, node_, config_tools_, framework_tools_, topic_tools_, transform_tools_, registration_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Mavros Manager created for agent: %s", agent_id_.c_str());
    }

    void onShutdown() override
    {
        // Destroy mavros manager
        mavros_manager_.reset();
    }

private: // Components
    // Agent ID
    ID agent_id_;
    // Mavros instance
    MavrosManager::SharedPtr mavros_manager_;
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
    auto node = std::make_shared<MavrosNode>("mavros_node", options);
    node->init();
    // Spin node
    rclcpp::spin(node);
    // Shutdown
    rclcpp::shutdown();
    return 0;
}