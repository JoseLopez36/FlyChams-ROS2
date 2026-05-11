#pragma once

// Tools includes
#include "flychams_common/settings/settings_tools.hpp"

// Core includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/ros_types.hpp"
#include "flychams_common/utils/ros_utils.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Simulation tools for controlling the simulation environment
     * (e.g. AirSim, Gazebo, etc.)
     *
     * @details
     * This class provides utilities for managing the communication
     * with the simulation environment.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class SimulationTools
    {
    public: // Constructors/Destructors
        SimulationTools(core::NodePtr node, const core::SettingsTools::SharedPtr& settings_tools)
            : node_(node), settings_tools_(settings_tools)
        {
            // Nothing to do
        }
        virtual ~SimulationTools() = default;
        virtual void shutdown() = 0;

    public: // Types
        using SharedPtr = std::shared_ptr<SimulationTools>;
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

    public: // Global control methods (override)
        virtual bool resetSimulation() = 0;
        virtual bool runSimulation() = 0;
        virtual bool pauseSimulation() = 0;

    public: // Window control methods (override)
        virtual void setWindows(const std::vector<WindowCmd>& window_cmds) = 0;
        virtual void drawWindow(const DrawCmd& draw_cmd) = 0;

    public: // Tracking control methods (override)
        virtual bool addTargetGroup(const core::IDs& target_ids, const std::vector<core::TargetType>& target_types, const std::vector<core::PointMsg>& positions, const bool& highlight, const std::vector<core::ColorMsg>& highlight_colors) = 0;
        virtual bool addClusterGroup(const core::IDs& cluster_ids, const std::vector<core::PointMsg>& centers, const std::vector<float>& radii, const bool& highlight, const std::vector<core::ColorMsg>& highlight_colors) = 0;
        virtual bool removeAllTargets() = 0;
        virtual bool removeAllClusters() = 0;
        virtual void updateTargetGroup(const core::IDs& target_ids, const std::vector<core::PointMsg>& positions) = 0;
        virtual void updateClusterGroup(const core::IDs& cluster_ids, const std::vector<core::PointMsg>& centers, const std::vector<float>& radii) = 0;

    protected: // Data
        // ROS components
        core::NodePtr node_;

        // Config tools
        core::SettingsTools::SharedPtr settings_tools_;
    };

    SimulationTools::SharedPtr createSimulationTools(core::NodePtr node, const core::SettingsTools::SharedPtr& settings_tools);

} // namespace flychams::simulation