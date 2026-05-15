#include "rclcpp/rclcpp.hpp"

// Registration includes
#include "flychams_coordinator/registration/element_registration.hpp"

// Core includes
#include "flychams_common/base/base_node.hpp"

using namespace flychams::core;
using namespace flychams::coordinator;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Registrator node for registering the different elements
 * in the simulation
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-02-28
 * ════════════════════════════════════════════════════════════════
 */
class ElementRegistratorNode : public BaseNode
{
public: // Constructor/Destructor
    ElementRegistratorNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Initialize element registration system
        element_registration_ = std::make_shared<ElementRegistration>(node_, settings_tools_, topic_tools_, transform_tools_, node_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Element registration created");
    }

    void onShutdown() override
    {
        // Destroy element registration system
        element_registration_.reset();
    }

private: // Components
    // Element registration system
    ElementRegistration::SharedPtr element_registration_;
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
    auto node = std::make_shared<ElementRegistratorNode>("element_registrator_node", options);
    node->init();
    // Spin node
    rclcpp::spin(node);
    // Shutdown
    rclcpp::shutdown();
    return 0;
}