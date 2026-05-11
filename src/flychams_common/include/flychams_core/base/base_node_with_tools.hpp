
#pragma once

// Tools includes
#include "flychams_common/settings/settings_tools.hpp"
#include "flychams_common/ros/topic_tools.hpp"
#include "flychams_common/ros/transform_tools.hpp"

// Core includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/ros_types.hpp"
#include "flychams_common/utils/ros_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base node with common functionality and tools
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-12
     * ════════════════════════════════════════════════════════════════
     */
    class BaseNodeWithTools : public rclcpp::Node
    {
    public: // Constructor/Destructor
        BaseNodeWithTools(const std::string& node_name, const rclcpp::NodeOptions& options);

        void init();

        virtual ~BaseNodeWithTools();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseNodeWithTools>;

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
        // Tools
        SettingsTools::SharedPtr settings_tools_;
        TopicTools::SharedPtr topic_tools_;
        TransformTools::SharedPtr transform_tools_;
    };

} // namespace flychams::core