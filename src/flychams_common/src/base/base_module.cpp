#include "flychams_common/base/base_module.hpp"

namespace flychams::core
{
    BaseModule::BaseModule(NodePtr node, SettingsTools::SharedPtr settings, CallbackGroupPtr module_cb_group)
        : node_(node), settings_(settings), module_cb_group_(module_cb_group)
    {
        // Nothing to do
    }

    void BaseModule::init()
    {
        // Initialize subscription options
        sub_options_with_module_cb_group_.callback_group = module_cb_group_;

        // Call on init overridable method
        onModuleInit();
    }

    BaseModule::~BaseModule()
    {
        shutdown();
    }

    void BaseModule::shutdown()
    {
        // Call on shutdown overridable method
        onModuleShutdown();
        // Destroy settings tools
        settings_.reset();
        // Destroy node
        node_.reset();
    }

} // namespace flychams::core

