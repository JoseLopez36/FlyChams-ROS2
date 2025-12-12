#include "rclcpp/rclcpp.hpp"

// Control includes
#include "flychams_bringup/settings/config_parser.hpp"

// Core includes
#include "flychams_core/base/base_node.hpp"

using namespace flychams::core;
using namespace flychams::bringup;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Config node for parsing configuration files to parameter
 *        server (ROS2)
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-12-12
 * ════════════════════════════════════════════════════════════════
 */
class ConfigParserNode : public BaseNode
{
public: // Constructor/Destructor
    ConfigParserNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Create config parser
        config_parser_ = std::make_shared<ConfigParser>(node_);

        RCLCPP_INFO(node_->get_logger(), "Config parser created");
    }

    void onShutdown() override
    {
        // Destroy config parser
        config_parser_.reset();
    }

private: // Components
    // Config parser
    ConfigParser::SharedPtr config_parser_;
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
    auto node = std::make_shared<ConfigParserNode>("config_parser_node", options);
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