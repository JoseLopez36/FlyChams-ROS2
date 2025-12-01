#include "rclcpp/rclcpp.hpp"

// Config tools includes
#include "flychams_core/config/config_tools.hpp"

using namespace flychams::core;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Settings node for creating configuration files for external
 *        tools and environments
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-12-01
 * ════════════════════════════════════════════════════════════════
 */

int main(int argc, char** argv)
{
    // Initialize ROS
    rclcpp::init(argc, argv);

    // Create node options
    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(true);

    // Create temporary node
    auto node = rclcpp::Node::make_shared("settings_node", options);

    // Create config tools
    auto config_tools = std::make_shared<ConfigTools>(node);

    // Generate all settings files
    RCLCPP_INFO(node->get_logger(), "Generating settings files...");

    // Create AirSim settings.json
    config_tools->createAirsimSettings();

    // Create agents.yaml for mission launch
    config_tools->createAgentsYaml();

    // Finish the node
    RCLCPP_INFO(node->get_logger(), "Settings files generated successfully");
    rclcpp::shutdown();
    return 0;
}

