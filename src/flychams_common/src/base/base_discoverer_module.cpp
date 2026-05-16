#include "flychams_common/base/base_discoverer_module.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor
// ════════════════════════════════════════════════════════════════════════════

BaseDiscovererModule::BaseDiscovererModule(BaseDiscovererNode::SharedPtr node)
    : node_(node)
{
    // Nothing to do
}

void BaseDiscovererModule::init()
{
    // Call on init overridable method
    onModuleInit();
}

BaseDiscovererModule::~BaseDiscovererModule()
{
    shutdown();
}

void BaseDiscovererModule::shutdown()
{
    // Call on shutdown overridable method
    onModuleShutdown();
    // Reset node
    node_.reset();
}