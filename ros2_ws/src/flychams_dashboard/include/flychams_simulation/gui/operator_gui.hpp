#pragma once

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::dashboard
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
    class GuiManager : public core::BaseModule
    {
    public: // Constructor/Destructor
        GuiManager(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<GuiManager>;
        enum class GuiMode
        {
            IDLE,
            RESET,
            TRACKING
        };
        struct WindowCmd
        {
            core::ID window_id;
            core::ID vehicle_id;
            core::ID camera_id;
            core::CropMsg crop;
            // Constructors
            WindowCmd() = default;
            WindowCmd(const core::ID& window_id_in, const core::ID& vehicle_id_in, const core::ID& camera_id_in)
            {
                window_id = window_id_in;
                vehicle_id = vehicle_id_in;
                camera_id = camera_id_in;
                crop.x = 0;
                crop.y = 0;
                crop.w = 0;
                crop.h = 0;
                crop.is_out_of_bounds = false;
            }
        };
        struct RectanglesCmd
        {
            std::vector<core::PointMsg> positions;
            std::vector<core::PointMsg> sizes;
            core::ColorMsg color;
            float thickness;
        };
        struct StringsCmd
        {
            std::vector<core::PointMsg> positions;
            std::vector<std::string> texts;
            core::ColorMsg color;
            float scale;
        };
        struct DrawCmd
        {
            core::ID window_id;
            // Rectangles
            RectanglesCmd rectangles;
            // Strings
            StringsCmd strings;

            // Constructor
            DrawCmd()
                : window_id(), rectangles(), strings()
            {
            }
        };
        struct Agent
        {
            // Status data
            core::AgentStatus status;
            bool has_status;
            // Setpoints data
            bool has_gui_setpoints;
            // Subscribers
            core::SubscriberPtr<core::AgentStatusMsg> status_sub;
            core::SubscriberPtr<core::GuiSetpointsMsg> gui_setpoints_sub;
            // Constructor
            Agent()
                : status(), has_status(false), has_gui_setpoints(false),
                status_sub(), gui_setpoints_sub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;

    private: // Data
        // GUI mode
        GuiMode gui_mode_;
        // Agent
        Agent agent_;
        // Window commands
        std::vector<WindowCmd> simulation_window_cmds_; // Simulation window commands
        std::vector<WindowCmd> operator_window_cmds_;   // Operator window commands
        DrawCmd central_draw_cmd_;                      // Central draw command

    public: // Public methods
        void activate() { gui_mode_ = GuiMode::RESET; }
        void deactivate() { gui_mode_ = GuiMode::IDLE; }

    private: // Callbacks
        void statusCallback(const core::AgentStatusMsg::SharedPtr msg);
        void setpointsCallback(const core::GuiSetpointsMsg::SharedPtr msg);

    private: // GUI management
        void update();

    private: // GUI methods
        void setWindows(const std::vector<WindowCmd>& window_cmds);
        void resetWindows(std::vector<WindowCmd>& window_cmds);
        void drawWindow(const DrawCmd& draw_cmd);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::simulation