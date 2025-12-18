#include "flychams_simulation/gui/simulation_gui.hpp"

using namespace flychams::core;

namespace flychams::simulation
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void SimulationGui::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "update_rate", 20.0f);

        // Initialize GUI mode
        gui_mode_ = GuiMode::IDLE;

        // Create simulation tools
        simulation_tools_ = createSimulationTools(node_, settings_tools_);

        // Get system config
        const auto& system_config = settings_tools_->getSystem();

        // Initialize window commands
        window_cmds_.clear();
        // Scenario window
        window_cmds_.push_back(WindowCmd(system_config.scenario_view_id, agent_id_, system_config.scenario_camera_id));
        // Agent window
        window_cmds_.push_back(WindowCmd(system_config.agent_view_id, agent_id_, system_config.agent_camera_id));
        // Payload window
        window_cmds_.push_back(WindowCmd(system_config.payload_view_id, agent_id_, system_config.payload_camera_id));
        // Map window
        window_cmds_.push_back(WindowCmd(system_config.map_view_id, agent_id_, system_config.map_camera_id));

        // Set update timer
        update_timer_ = RosUtils::createTimer(node_, update_rate_,
            std::bind(&SimulationGui::update, this), module_cb_group_);
    }

    void SimulationGui::onShutdown()
    {
        // Destroy simulation tools
        simulation_tools_.reset();
        // Destroy update timer
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE: Update simulation GUI
    // ════════════════════════════════════════════════════════════════════════════

    void SimulationGui::update()
    {
        switch (gui_mode_)
        {
        case GuiMode::IDLE:
            // In this mode, we do nothing
            break;

        case GuiMode::RUNNING:
            // Set simulation windows
            setWindows(window_cmds_);

            // Set mode to IDLE
            gui_mode_ = GuiMode::IDLE;
            break;

        default:
            RCLCPP_ERROR(node_->get_logger(), "Simulation GUI: Invalid GUI mode: %d", static_cast<int>(gui_mode_));
            break;
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // IMPLEMENTATION: Implementation methods for GUI management
    // ════════════════════════════════════════════════════════════════════════════

    void SimulationGui::setWindows(const std::vector<WindowCmd>& window_cmds)
    {
        // Send commands to set window images
        simulation_tools_->setWindows(window_cmds);
        // Delay to ensure GUI is updated
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

} // namespace flychams::simulation