#include "flychams_simulation/tools/simulation_tools.hpp"

// Derived classes includes
#include "flychams_simulation/tools/airsim_tools.hpp"

using namespace flychams::core;

namespace flychams::simulation
{
    SimulationTools::SharedPtr createSimulationTools(NodePtr node, const SettingsTools::SharedPtr& settings_tools)
    {
        // Get framework
        const SimulationFramework framework = settings_tools->getSystem().simulation_framework;

        // Create framework tools based on simulation framework
        switch (framework)
        {
        case SimulationFramework::AirSim:
            return std::make_shared<AirsimTools>(node, settings_tools);
        default:
            throw std::runtime_error("Unknown simulation framework: " + std::to_string(static_cast<int>(framework)));
        }
    }

} // namespace flychams::simulation