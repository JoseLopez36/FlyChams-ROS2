#include "flychams_common/base/base_module.hpp"

namespace flychams::common
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor
    // ════════════════════════════════════════════════════════════════════════════

    BaseModule::BaseModule(BaseNode::SharedPtr node)
        : node_(node)
    {
        // Nothing to do
    }

    void BaseModule::init()
    {
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
        // Reset node
        node_.reset();
    }

} // namespace flychams::common