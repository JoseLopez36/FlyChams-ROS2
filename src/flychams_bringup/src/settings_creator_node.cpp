#include "rclcpp/rclcpp.hpp"

// Settings includes
#include "flychams_core/settings/spreadsheet_parser.hpp"
#include "flychams_core/settings/mission_settings_creator.hpp"
#include "flychams_core/settings/airsim_settings_creator.hpp"

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/config_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/math_utils.hpp"
#include "flychams_core/utils/ros_utils.hpp"
#include "flychams_core/settings/mission_settings_parser.hpp"

using namespace flychams::core;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Settings node for creating settings files for external
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
    auto node = rclcpp::Node::make_shared("settings_creator_node", options);

    // Get path to configuration spreadsheet
    const std::string& spreadsheet_path = RosUtils::getParameter<std::string>(node, "path.config_spreadsheet_path");

    // Parse mission configuration
    MissionConfigPtr config_ptr;
    try
    {
        RCLCPP_INFO(node->get_logger(), "Parsing mission configuration: %s", spreadsheet_path.c_str());
        config_ptr = SpreadsheetParser::parseSpreadsheet(spreadsheet_path);
        RCLCPP_INFO(node->get_logger(), "Mission configuration parsed successfully");
    }
    catch (const std::exception& e)
    {
        RCLCPP_ERROR(node->get_logger(), "Error parsing mission configuration: %s", e.what());
        rclcpp::shutdown();
    }

    // Parse system, topics and frames parameters from ROS2 parameters server
    MissionSettingsParser::parseSystemParameters(node, config_ptr);
    MissionSettingsParser::parseTopicParameters(node, config_ptr);
    MissionSettingsParser::parseFrameParameters(node, config_ptr);

    // Generate all settings files
    RCLCPP_INFO(node->get_logger(), "Creating settings files...");

    // Create mission.yaml file
    const std::string& mission_settings_path = RosUtils::getParameter<std::string>(node, "path.mission_settings_path");
    MissionSettingsCreator::createMissionSettings(config_ptr, mission_settings_path);

    // Create AirSim settings
    const std::string& airsim_settings_path = RosUtils::getParameter<std::string>(node, "path.airsim_settings_path");
    AirsimSettingsCreator::createAirsimSettings(config_ptr, airsim_settings_path);

    // Finish the node
    RCLCPP_INFO(node->get_logger(), "Settings files created successfully");
    rclcpp::shutdown();
    return 0;
}

