#include "rclcpp/rclcpp.hpp"

// Camera includes
#include "flychams_simulation/camera/camera_manager.hpp"

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::simulation;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Camera node for managing agent cameras activation in the
 * simulation
 *
 * @details
 * This class implements the camera node for managing agent cameras
 * activation in the simulation. It uses the discoverer node to
 * discover agents and manages their camera activation state through
 * the AirSim CameraCapture service.
 *
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-05-13
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
        // Use callback group from discovery node (to avoid race conditions)
        // Initialize camera control
        camera_control_ = std::make_shared<CameraControl>(node_, settings_tools_, topic_tools_, transform_tools_, discovery_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Camera control created");
    }

    void onShutdown() override
    {
        // Destroy camera control
        camera_control_.reset();
    }

private: // Element management
    void onAddAgent(const ID& agent_id) override
    {
        // Add agent to camera control
        camera_control_->addAgent(agent_id);
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Remove agent from camera control
        camera_control_->removeAgent(agent_id);
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
    // Camera control
    CameraControl::SharedPtr camera_control_;
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
    auto node = std::make_shared<CameraControlNode>("camera_manager_node", options);
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