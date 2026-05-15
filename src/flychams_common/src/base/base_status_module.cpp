#include "flychams_common/base/base_status_module.hpp"

namespace flychams::core
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor
    // ════════════════════════════════════════════════════════════════════════════

    BaseStatusModule::BaseStatusModule(BaseStatusNode::SharedPtr node, SettingsTools::SharedPtr settings, CallbackGroupPtr module_cb_group)
        : BaseModule(node, settings, module_cb_group), status_node_(node)
    {
        // Nothing to do
    }

} // namespace flychams::core