#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

BaseDiscovererNode::BaseDiscovererNode(const std::string& node_name, const rclcpp::NodeOptions& options)
    : BaseStatusNode(node_name, options)
{
    // Nothing to do
}

void BaseDiscovererNode::onStatusInit()
{
    // Initialize elements map
    elements_.clear();

    // Initialize discovery subscriber
    discovery_sub_ = createRegistrationSubscriber(
        std::bind(&BaseDiscovererNode::onDiscovery, this, std::placeholders::_1),
        sub_options_with_node_cb_group_);

    // Call on discovery init overridable method
    onDiscoveryInit();
}

void BaseDiscovererNode::onStatusShutdown()
{
    // Call on discovery shutdown overridable method
    onDiscoveryShutdown();
    // Destroy discovery subscriber
    discovery_sub_.reset();
    // Destroy elements map
    elements_.clear();
}

void BaseDiscovererNode::onDiscovery(const RegistrationMsg::SharedPtr msg)
{
    // Track elements in this message
    std::unordered_set<ID> current_elements;
    for (const auto& element : msg->elements)
    {
        // Get element id and type
        const auto element_id = element.id;
        const auto element_type = static_cast<ElementType>(element.type);

        // Add element ID to current set
        current_elements.insert(element_id);

        // Add element if it doesn't exist already
        if (elements_.find(element_id) == elements_.end())
        {
            elements_.insert({ element_id, element_type });

            // Call corresponding add callback
            switch (element_type)
            {
            case ElementType::Agent:
                // Call onAddAgent callback
                onAddAgent(element_id);
                break;

            case ElementType::Target:
                // Call onAddTarget callback
                onAddTarget(element_id);
                break;

            case ElementType::Cluster:
                // Call onAddCluster callback
                onAddCluster(element_id);
                break;

            case ElementType::None:
                // Unexpected element type - log warning
                RCLCPP_WARN(get_logger(), "Cannot add element %s: invalid element type 'None'", element_id.c_str());
                break;
            }
        }
    }

    // Remove elements that are no longer present
    std::vector<ID> to_remove;
    for (const auto& [element_id, element_type] : elements_)
    {
        if (current_elements.find(element_id) == current_elements.end())
        {
            to_remove.push_back(element_id);
        }
    }

    // Remove elements that are not in the current message
    for (const auto& element_id : to_remove)
    {
        const auto element_type = elements_[element_id];
        elements_.erase(element_id);

        // Call corresponding remove callback
        switch (element_type)
        {
        case ElementType::Agent:
            // Call onRemoveAgent callback
            onRemoveAgent(element_id);
            break;

        case ElementType::Target:
            // Call onRemoveTarget callback
            onRemoveTarget(element_id);
            break;

        case ElementType::Cluster:
            // Call onRemoveCluster callback
            onRemoveCluster(element_id);
            break;

        case ElementType::None:
            // Unexpected element type - log warning
            RCLCPP_WARN(get_logger(), "Cannot remove element %s: invalid element type 'None'", element_id.c_str());
            break;
        }
    }
}