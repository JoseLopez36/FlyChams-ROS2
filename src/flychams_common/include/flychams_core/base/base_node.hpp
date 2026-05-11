#pragma once

// Core includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/ros_types.hpp"
#include "flychams_common/utils/ros_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base node with common functionality
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class BaseNode : public rclcpp::Node
    {
    public: // Constructor/Destructor
        BaseNode(const std::string& node_name, const rclcpp::NodeOptions& options);
        
        void init();

        virtual ~BaseNode();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseNode>;

    protected: // Overridable methods
        virtual void onInit() {}
        virtual void onShutdown() {}

    protected: // Components
        // Node
        NodePtr node_;
        const std::string node_name_;
        // Callback group
        CallbackGroupPtr node_cb_group_;
        rclcpp::SubscriptionOptions sub_options_with_node_cb_group_;
    };

} // namespace flychams::core