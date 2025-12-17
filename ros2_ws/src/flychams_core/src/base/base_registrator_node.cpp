#include "flychams_core/base/base_registrator_node.hpp"

namespace flychams::core
{
    BaseRegistratorNode::BaseRegistratorNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : Node(node_name, options), node_name_(node_name)
    {
        // Nothing to do
    }

    void BaseRegistratorNode::init()
    {
        // Get node pointer
        node_ = this->shared_from_this();
        RCLCPP_INFO(node_->get_logger(), "Starting %s node...", node_name_.c_str());

        // Create tools
        settings_tools_ = std::make_shared<SettingsTools>(node_);
        topic_tools_ = std::make_shared<TopicTools>(node_, settings_tools_);
        transform_tools_ = std::make_shared<TransformTools>(node_, settings_tools_);

        // Create callback group
        registration_cb_group_ = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        sub_options_with_registration_cb_group_.callback_group = registration_cb_group_;

        // Initialize registration publisher
        elements_.clear();
        registration_pub_ = topic_tools_->createRegistrationPublisher();

        // Initialize update timer (1 Hz)
        update_timer_ = RosUtils::createWallTimer(node_, 1.0f, [this]() { publishRegistration(); }, registration_cb_group_);

        // Call on init overridable method
        onInit();
        RCLCPP_INFO(node_->get_logger(), "%s node running", node_name_.c_str());
    }

    BaseRegistratorNode::~BaseRegistratorNode()
    {
        shutdown();
    }

    void BaseRegistratorNode::shutdown()
    {
        RCLCPP_INFO(node_->get_logger(), "Shutting down %s node...", node_name_.c_str());
        // Call on shutdown overridable method
        onShutdown();
        // Destroy update timer
        update_timer_.reset();
        // Destroy registration publisher
        elements_.clear();
        registration_pub_.reset();
        // Destroy tools
        settings_tools_.reset();
        topic_tools_.reset();
        transform_tools_.reset();
    }

    void BaseRegistratorNode::registerElement(const ID& element_id, const ElementType& element_type)
    {
        // Add element to map (only if not already registered)
        if (elements_.find(element_id) != elements_.end())
            return;
        elements_.insert({ element_id, element_type });

        RCLCPP_INFO(node_->get_logger(), "Element %s registered", element_id.c_str());
    }

    void BaseRegistratorNode::unregisterElement(const ID& element_id, const ElementType& element_type)
    {
        // Remove element from map (only if registered)
        if (elements_.find(element_id) == elements_.end())
            return;
        elements_.erase(element_id);

        RCLCPP_INFO(node_->get_logger(), "Element %s unregistered", element_id.c_str());
    }

    void BaseRegistratorNode::publishRegistration()
    {
        // Create and publish registration message
        RegistrationMsg msg;
        for (const auto& [id, type] : elements_)
        {
            ElementMsg element;
            element.id = id;
            element.type = static_cast<uint8_t>(type);
            msg.elements.push_back(element);
        }
        registration_pub_->publish(msg);
    }

} // namespace flychams::core

