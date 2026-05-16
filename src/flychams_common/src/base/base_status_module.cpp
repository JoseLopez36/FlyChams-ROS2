#include "flychams_common/base/base_status_module.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor
// ════════════════════════════════════════════════════════════════════════════

BaseStatusModule::BaseStatusModule(BaseStatusNode::SharedPtr node)
    : node_(node)
{
    // Nothing to do
}

void BaseStatusModule::init()
{
    // Call on init overridable method
    onModuleInit();
}

BaseStatusModule::~BaseStatusModule()
{
    shutdown();
}

void BaseStatusModule::shutdown()
{
    // Call on shutdown overridable method
    onModuleShutdown();
    // Reset node
    node_.reset();
}