#pragma once

// Simulation tools include
#include "flychams_simulation/tools/simulation_tools.hpp"

// Tools includes
#include "flychams_core/settings/settings_tools.hpp"

// AirSim interfaces includes
// Global commands
#include <airsim_interfaces/srv/reset.hpp>
#include <airsim_interfaces/srv/run.hpp>
#include <airsim_interfaces/srv/pause.hpp>
// Window commands
#include <airsim_interfaces/msg/window_image_cmd_group.hpp>
#include <airsim_interfaces/msg/window_rectangle_cmd.hpp>
#include <airsim_interfaces/msg/window_string_cmd.hpp>
// Tracking commands
#include <airsim_interfaces/srv/add_target_group.hpp>
#include <airsim_interfaces/srv/add_cluster_group.hpp>
#include <airsim_interfaces/srv/remove_all_targets.hpp>
#include <airsim_interfaces/srv/remove_all_clusters.hpp>
#include <airsim_interfaces/msg/update_target_cmd_group.hpp>
#include <airsim_interfaces/msg/update_cluster_cmd_group.hpp>

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief AirSim-specific implementation of the simulation tools
     * utilities
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-14
     * ════════════════════════════════════════════════════════════════
     */
    class AirsimTools : public SimulationTools
    {
    public: // Constructors/Destructors
        AirsimTools(core::NodePtr node, const core::SettingsTools::SharedPtr& settings_tools);
        ~AirsimTools() override;
        void shutdown() override;

    public: // Types
        using ResetSrv = airsim_interfaces::srv::Reset;
        using RunSrv = airsim_interfaces::srv::Run;
        using PauseSrv = airsim_interfaces::srv::Pause;
        using WindowImageCmdGroupMsg = airsim_interfaces::msg::WindowImageCmdGroup;
        using WindowRectangleCmdMsg = airsim_interfaces::msg::WindowRectangleCmd;
        using WindowStringCmdMsg = airsim_interfaces::msg::WindowStringCmd;
        using AddTargetGroupSrv = airsim_interfaces::srv::AddTargetGroup;
        using AddClusterGroupSrv = airsim_interfaces::srv::AddClusterGroup;
        using RemoveAllTargetsSrv = airsim_interfaces::srv::RemoveAllTargets;
        using RemoveAllClustersSrv = airsim_interfaces::srv::RemoveAllClusters;
        using UpdateTargetCmdGroupMsg = airsim_interfaces::msg::UpdateTargetCmdGroup;
        using UpdateClusterCmdGroupMsg = airsim_interfaces::msg::UpdateClusterCmdGroup;

    public: // Global control methods
        bool resetSimulation() override;
        bool runSimulation() override;
        bool pauseSimulation() override;

    public: // Window control methods
        void setWindows(const std::vector<WindowCmd>& window_cmds) override;
        void drawWindow(const DrawCmd& draw_cmd) override;

    public: // Tracking control methods
        bool addTargetGroup(const core::IDs& target_ids, const std::vector<core::TargetType>& target_types, const std::vector<core::PointMsg>& positions, const bool& highlight, const std::vector<core::ColorMsg>& highlight_colors) override;
        bool addClusterGroup(const core::IDs& cluster_ids, const std::vector<core::PointMsg>& centers, const std::vector<float>& radii, const bool& highlight, const std::vector<core::ColorMsg>& highlight_colors) override;
        bool removeAllTargets() override;
        bool removeAllClusters() override;
        void updateTargetGroup(const core::IDs& target_ids, const std::vector<core::PointMsg>& positions) override;
        void updateClusterGroup(const core::IDs& cluster_ids, const std::vector<core::PointMsg>& centers, const std::vector<float>& radii) override;

    private: // Utility methods
        int getWindowIndex(const core::ID& window_id) const
        {
            // Get system config
            const auto& system_config = settings_tools_->getSystem();

            if (window_id == system_config.scenario_view_id)
                return 0;
            else if (window_id == system_config.agent_view_id)
                return 1;
            else if (window_id == system_config.map_view_id)
                return 2;
            else if (window_id == system_config.payload_view_id)
                return 3;
            else if (window_id == system_config.tracking_view_ids.at(0))
                return 4;
            else if (window_id == system_config.tracking_view_ids.at(1))
                return 5;
            else if (window_id == system_config.tracking_view_ids.at(2))
                return 6;
            else if (window_id == system_config.tracking_view_ids.at(3))
                return 7;
            else if (window_id == system_config.tracking_view_ids.at(4))
                return 8;
            else
            {
                RCLCPP_ERROR(node_->get_logger(), "Invalid window ID: %s", window_id.c_str());
                return 0;
            }
        }

    private: // ROS components
        // Global commands
        core::ClientPtr<ResetSrv> reset_client_;
        core::ClientPtr<RunSrv> run_client_;
        core::ClientPtr<PauseSrv> pause_client_;

        // Window commands
        core::PublisherPtr<WindowImageCmdGroupMsg> window_image_cmd_group_pub_;
        core::PublisherPtr<WindowRectangleCmdMsg> window_rectangle_cmd_pub_;
        core::PublisherPtr<WindowStringCmdMsg> window_string_cmd_pub_;

        // Tracking commands
        core::ClientPtr<AddTargetGroupSrv> add_target_group_client_;
        core::ClientPtr<AddClusterGroupSrv> add_cluster_group_client_;
        core::ClientPtr<RemoveAllTargetsSrv> remove_all_targets_client_;
        core::ClientPtr<RemoveAllClustersSrv> remove_all_clusters_client_;
        core::PublisherPtr<UpdateTargetCmdGroupMsg> update_target_cmd_group_pub_;
        core::PublisherPtr<UpdateClusterCmdGroupMsg> update_cluster_cmd_group_pub_;
    };

} // namespace flychams::simulation