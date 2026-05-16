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
    class SimulationBridge : public common::BaseModule
    {
    public: // Constructors/Destructors
        SimulationBridge(common::BaseNode::SharedPtr node)
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
        bool addTargetGroup(const common::IDs& target_ids, const std::vector<common::TargetType>& target_types, const std::vector<common::PointMsg>& positions, const bool& highlight, const std::vector<common::ColorMsg>& highlight_colors);
        bool addClusterGroup(const common::IDs& cluster_ids, const std::vector<common::PointMsg>& centers, const std::vector<float>& radii, const bool& highlight, const std::vector<common::ColorMsg>& highlight_colors);
        bool removeAllTargets();
        bool removeAllClusters();
        void updateTargetGroup(const common::IDs& target_ids, const std::vector<common::PointMsg>& positions);
        void updateClusterGroup(const common::IDs& cluster_ids, const std::vector<common::PointMsg>& centers, const std::vector<float>& radii);

    private: // ROS components
        // Global commands
        common::ClientPtr<ResetSrv> reset_client_;
        common::ClientPtr<RunSrv> run_client_;
        common::ClientPtr<PauseSrv> pause_client_;
        // Tracking commands
        common::ClientPtr<AddTargetGroupSrv> add_target_group_client_;
        common::ClientPtr<AddClusterGroupSrv> add_cluster_group_client_;
        common::ClientPtr<RemoveAllTargetsSrv> remove_all_targets_client_;
        common::ClientPtr<RemoveAllClustersSrv> remove_all_clusters_client_;
        common::PublisherPtr<UpdateTargetCmdGroupMsg> update_target_cmd_group_pub_;
        common::PublisherPtr<UpdateClusterCmdGroupMsg> update_cluster_cmd_group_pub_;
    };

} // namespace flychams::simulation