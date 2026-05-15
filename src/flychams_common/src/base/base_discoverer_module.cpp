#include "flychams_common/base/base_discoverer_module.hpp"

namespace flychams::core
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor
    // ════════════════════════════════════════════════════════════════════════════

    BaseDiscovererModule::BaseDiscovererModule(BaseDiscovererNode::SharedPtr node, SettingsTools::SharedPtr settings, CallbackGroupPtr module_cb_group)
        : BaseStatusModule(node, settings, module_cb_group), discoverer_node_(node)
    {
        // Nothing to do
    }

} // namespace flychams::core