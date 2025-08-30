#include "flychams_core/framework/framework_tools.hpp"

// Derived classes includes
#include "flychams_core/framework/airsim_tools.hpp"

namespace flychams::core
{
    FrameworkTools::SharedPtr createFrameworkTools(NodePtr node, const ConfigTools::SharedPtr& config_tools)
    {
        // Get framework
        const SimulationFramework framework = config_tools->getSystem().framework;

        // Create framework tools based on simulation framework
        switch (framework)
        {
        case SimulationFramework::AirSim:
            return std::make_shared<AirsimTools>(node, config_tools);
        default:
            throw std::runtime_error("Unknown simulation framework: " + std::to_string(static_cast<int>(framework)));
        }
    }

} // namespace flychams::core