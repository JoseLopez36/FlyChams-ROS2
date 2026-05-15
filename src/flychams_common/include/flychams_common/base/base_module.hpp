#pragma once

// Settings include
#include "flychams_common/settings/settings_tools.hpp"

// Types includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/config_types.hpp"
#include "flychams_common/types/ros_types.hpp"

// Utils includes
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"
#include "flychams_common/utils/ros_utils.hpp"

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
        BaseModule(NodePtr node, SettingsTools::SharedPtr settings, CallbackGroupPtr module_cb_group);

        void init();

        virtual ~BaseModule();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseModule>;

    protected: // Overridable methods
        virtual void onModuleInit() {}
        virtual void onModuleShutdown() {}

    private: // Settings data
        SettingsTools::SharedPtr settings_;

    private: // ROS components
        // Node
        NodePtr node_;
        // Callback group
        CallbackGroupPtr module_cb_group_;
        rclcpp::SubscriptionOptions sub_options_with_module_cb_group_;
    };

} // namespace flychams::core