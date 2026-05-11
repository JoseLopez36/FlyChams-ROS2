#include "flychams_common/base/base_node_with_tools.hpp"

namespace flychams::core
{
    BaseNodeWithTools::BaseNodeWithTools(const std::string& node_name, const rclcpp::NodeOptions& options)
        : Node(node_name, options), node_name_(node_name)
    {
        // Nothing to do
    }

    void BaseNodeWithTools::init()
    {
        // Get node pointer
        node_ = this->shared_from_this();
        RCLCPP_INFO(node_->get_logger(), "Starting %s node...", node_name_.c_str());

        // Create callback group
        node_cb_group_ = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        sub_options_with_node_cb_group_.callback_group = node_cb_group_;

        // Create tools
        settings_tools_ = std::make_shared<SettingsTools>(node_);
        topic_tools_ = std::make_shared<TopicTools>(node_, settings_tools_);
        transform_tools_ = std::make_shared<TransformTools>(node_, settings_tools_);

        // Call on init overridable method
        onInit();
        RCLCPP_INFO(node_->get_logger(), "%s node running", node_name_.c_str());
    }

    BaseNodeWithTools::~BaseNodeWithTools()
    {
        shutdown();
    }

    void BaseNodeWithTools::shutdown()
    {
        RCLCPP_INFO(node_->get_logger(), "Shutting down %s node...", node_name_.c_str());
        // Call on shutdown overridable method
        onShutdown();
        // Destroy tools
        settings_tools_.reset();
        topic_tools_.reset();
        transform_tools_.reset();
    }

} // namespace flychams::core