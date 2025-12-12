#include "flychams_core/base/base_module.hpp"

namespace flychams::core
{
    BaseModule::BaseModule(NodePtr node, SettingsTools::SharedPtr config_tools, TopicTools::SharedPtr topic_tools, TransformTools::SharedPtr transform_tools, CallbackGroupPtr module_cb_group)
        : node_(node), config_tools_(config_tools), topic_tools_(topic_tools), transform_tools_(transform_tools), module_cb_group_(module_cb_group)
    {
        // Nothing to do
    }

    void BaseModule::init()
    {
        // Initialize subscription options
        sub_options_with_module_cb_group_.callback_group = module_cb_group_;

        // Call on init overridable method
        onInit();
    }

    BaseModule::~BaseModule()
    {
        shutdown();
    }

    void BaseModule::shutdown()
    {
        // Call on shutdown overridable method
        onShutdown();
        // Destroy tools
        config_tools_.reset();
        topic_tools_.reset();
        transform_tools_.reset();
        // Destroy node
        node_.reset();
    }

} // namespace flychams::core

