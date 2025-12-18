#pragma once

// Simulation tools include
#include "flychams_simulation/tools/simulation_tools.hpp"

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Manager for GUI
     *
     * @details
     * This class is responsible for controlling the different windows
     * in the GUI. It manages a single agent and its respective parameters.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-27
     * ════════════════════════════════════════════════════════════════
     */
    class SimulationGui : public core::BaseModule
    {
    public: // Constructor/Destructor
        SimulationGui(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<SimulationGui>;
        using WindowCmd = SimulationTools::WindowCmd;
        enum class GuiMode
        {
            IDLE,
            RESET,
            RUNNING
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;

    private: // Data
        // GUI mode
        GuiMode gui_mode_;
        // Simulation tools
        SimulationTools::SharedPtr simulation_tools_;
        // Window commands
        std::vector<WindowCmd> window_cmds_; // Window commands

    public: // Public methods
        void stop() { gui_mode_ = GuiMode::IDLE; }
        void start() { gui_mode_ = GuiMode::RUNNING; }

    private: // GUI management
        void update();

    private: // GUI methods
        void setWindows(const std::vector<WindowCmd>& window_cmds);
        void resetWindows(std::vector<WindowCmd>& window_cmds);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::simulation