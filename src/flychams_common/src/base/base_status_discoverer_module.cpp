#include "flychams_common/base/base_status_discoverer_module.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor
// ════════════════════════════════════════════════════════════════════════════

BaseStatusDiscovererModule::BaseStatusDiscovererModule(BaseStatusDiscovererNode::SharedPtr node)
    : node_(node)
{
    // Nothing to do
}

void BaseStatusDiscovererModule::init()
{
    // Call on init overridable method
    onModuleInit();
}

BaseStatusDiscovererModule::~BaseStatusDiscovererModule()
{
    shutdown();
}

void BaseStatusDiscovererModule::shutdown()
{
    // Call on shutdown overridable method
    onModuleShutdown();
    // Reset node
    node_.reset();
}