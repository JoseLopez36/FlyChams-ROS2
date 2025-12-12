#pragma once

// Tools includes
#include "flychams_core/settings/settings_tools.hpp"
#include "flychams_core/ros/topic_tools.hpp"
#include "flychams_core/ros/transform_tools.hpp"

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/config_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/math_utils.hpp"
#include "flychams_core/utils/tf_utils.hpp"
#include "flychams_core/utils/vision_utils.hpp"
#include "flychams_core/utils/ros_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base module for all sub-nodes in the system
     *
     * @details
     * This class is the base class for all sub-nodes present in the
     * system. It provides a common interface for all sub-nodes and a
     * set of utilities for the modules to use.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class BaseModule
    {
    public: // Constructor/Destructor
        BaseModule(NodePtr node, SettingsTools::SharedPtr config_tools, TopicTools::SharedPtr topic_tools, TransformTools::SharedPtr transform_tools, CallbackGroupPtr module_cb_group);

        void init();

        virtual ~BaseModule();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseModule>;

    protected: // Overridable methods
        virtual void onInit() {}
        virtual void onShutdown() {}

    protected: // Components
        // Node
        NodePtr node_;
        // Tools
        SettingsTools::SharedPtr config_tools_;
        TopicTools::SharedPtr topic_tools_;
        TransformTools::SharedPtr transform_tools_;
        // Callback group
        CallbackGroupPtr module_cb_group_;
        // Subscription options
        rclcpp::SubscriptionOptions sub_options_with_module_cb_group_;
    };

} // namespace flychams::core