#pragma once

// AirSim interfaces includes
// Global commands
#include <airsim_interfaces/srv/reset.hpp>
#include <airsim_interfaces/srv/run.hpp>
#include <airsim_interfaces/srv/pause.hpp>
// Tracking commands
#include <airsim_interfaces/srv/add_target_group.hpp>
#include <airsim_interfaces/srv/add_cluster_group.hpp>
#include <airsim_interfaces/srv/remove_all_targets.hpp>
#include <airsim_interfaces/srv/remove_all_clusters.hpp>
#include <airsim_interfaces/msg/update_target_cmd_group.hpp>
#include <airsim_interfaces/msg/update_cluster_cmd_group.hpp>

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Simulation tools for controlling the simulation environment
     * via AirSim
     *
     * @details
     * This class provides utilities for managing the communication
     * with the AirSim simulation environment.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class SimulationBridge : public core::BaseModule
    {
    public: // Constructors/Destructors
        SimulationBridge(core::BaseNode::SharedPtr node)
            : BaseModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<SimulationBridge>;
        using ResetSrv = airsim_interfaces::srv::Reset;
        using RunSrv = airsim_interfaces::srv::Run;
        using PauseSrv = airsim_interfaces::srv::Pause;
        using AddTargetGroupSrv = airsim_interfaces::srv::AddTargetGroup;
        using AddClusterGroupSrv = airsim_interfaces::srv::AddClusterGroup;
        using RemoveAllTargetsSrv = airsim_interfaces::srv::RemoveAllTargets;
        using RemoveAllClustersSrv = airsim_interfaces::srv::RemoveAllClusters;
        using UpdateTargetCmdGroupMsg = airsim_interfaces::msg::UpdateTargetCmdGroup;
        using UpdateClusterCmdGroupMsg = airsim_interfaces::msg::UpdateClusterCmdGroup;

    public: // Global control methods
        bool resetSimulation();
        bool runSimulation();
        bool pauseSimulation();

    public: // Tracking control methods
        bool addTargetGroup(const core::IDs& target_ids, const std::vector<core::TargetType>& target_types, const std::vector<core::PointMsg>& positions, const bool& highlight, const std::vector<core::ColorMsg>& highlight_colors);
        bool addClusterGroup(const core::IDs& cluster_ids, const std::vector<core::PointMsg>& centers, const std::vector<float>& radii, const bool& highlight, const std::vector<core::ColorMsg>& highlight_colors);
        bool removeAllTargets();
        bool removeAllClusters();
        void updateTargetGroup(const core::IDs& target_ids, const std::vector<core::PointMsg>& positions);
        void updateClusterGroup(const core::IDs& cluster_ids, const std::vector<core::PointMsg>& centers, const std::vector<float>& radii);

    private: // ROS components
        // Global commands
        core::ClientPtr<ResetSrv> reset_client_;
        core::ClientPtr<RunSrv> run_client_;
        core::ClientPtr<PauseSrv> pause_client_;
        // Tracking commands
        core::ClientPtr<AddTargetGroupSrv> add_target_group_client_;
        core::ClientPtr<AddClusterGroupSrv> add_cluster_group_client_;
        core::ClientPtr<RemoveAllTargetsSrv> remove_all_targets_client_;
        core::ClientPtr<RemoveAllClustersSrv> remove_all_clusters_client_;
        core::PublisherPtr<UpdateTargetCmdGroupMsg> update_target_cmd_group_pub_;
        core::PublisherPtr<UpdateClusterCmdGroupMsg> update_cluster_cmd_group_pub_;
    };

} // namespace flychams::simulation