#include "rclcpp/rclcpp.hpp"

// Control includes
#include "flychams_control/camera/camera_control.hpp"

// Core includes
#include "flychams_core/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::control;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Control node for controlling the agent's cameras (gimbal/
 * camera)
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-02-28
 * ════════════════════════════════════════════════════════════════
 */
class CameraControlNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    CameraControlNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Initialize camera controllers
        camera_control_.clear();
    }

    void onShutdown() override
    {
        // Destroy camera controllers
        camera_control_.clear();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Create callback group for camera control unit
        auto control_cb_group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

        // Create camera controller
        auto camera_control = std::make_shared<CameraControl>(agent_id, node_, config_tools_, framework_tools_, topic_tools_, transform_tools_, control_cb_group);
        camera_control_.insert(std::make_pair(agent_id, camera_control));
        RCLCPP_INFO(node_->get_logger(), "Camera controller created for agent %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Destroy camera controller
        camera_control_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Camera controller destroyed for agent %s", agent_id.c_str());
    }

private: // Components
    // Camera controllers
    std::unordered_map<ID, CameraControl::SharedPtr> camera_control_;
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
    auto node = std::make_shared<CameraControlNode>("camera_control_node", options);
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