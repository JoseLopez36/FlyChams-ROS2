#include "rclcpp/rclcpp.hpp"

// Settings includes
#include "flychams_common/settings/mission_settings_parser.hpp"
#include "flychams_common/settings/spreadsheet_parser.hpp"
#include "flychams_common/settings/mission_settings_creator.hpp"
#include "flychams_common/settings/airsim_settings_creator.hpp"

// Types includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/config_types.hpp"
#include "flychams_common/types/ros_types.hpp"

// Utils includes
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"

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
    std::string spreadsheet_path;
    node->get_parameter("path.config_spreadsheet_path", spreadsheet_path);

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
    std::string mission_settings_path;
    node->get_parameter("path.mission_settings_path", mission_settings_path);
    MissionSettingsCreator::createMissionSettings(config_ptr, mission_settings_path);

    // Create AirSim settings
    std::string airsim_settings_path;
    node->get_parameter("path.airsim_settings_path", airsim_settings_path);
    AirsimSettingsCreator::createAirsimSettings(config_ptr, airsim_settings_path);

    // Finish the node
    RCLCPP_INFO(node->get_logger(), "Settings files created successfully");
    rclcpp::shutdown();
    return 0;
}

