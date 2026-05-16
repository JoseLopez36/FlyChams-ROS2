#include "flychams_simulation/bridge/simulation_bridge.hpp"

using namespace flychams::common;

using namespace flychams::simulation;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void SimulationBridge::onModuleInit()
{
    // Initialize ROS components
    // Global commands
    reset_client_ = node_->create_client<ResetSrv>("/airsim/reset");
    run_client_ = node_->create_client<RunSrv>("/airsim/run");
    pause_client_ = node_->create_client<PauseSrv>("/airsim/pause");
    // Tracking commands
    add_target_group_client_ = node_->create_client<AddTargetGroupSrv>("/airsim/targets/cmd/add");
    add_cluster_group_client_ = node_->create_client<AddClusterGroupSrv>("/airsim/clusters/cmd/add");
    remove_all_targets_client_ = node_->create_client<RemoveAllTargetsSrv>("/airsim/targets/cmd/remove_all");
    remove_all_clusters_client_ = node_->create_client<RemoveAllClustersSrv>("/airsim/clusters/cmd/remove_all");
    update_target_cmd_group_pub_ = node_->create_publisher<UpdateTargetCmdGroupMsg>("/airsim/targets/cmd/update", 10);
    update_cluster_cmd_group_pub_ = node_->create_publisher<UpdateClusterCmdGroupMsg>("/airsim/clusters/cmd/update", 10);
}

void SimulationBridge::onModuleShutdown()
{
    // Destroy clients
    reset_client_.reset();
    run_client_.reset();
    pause_client_.reset();
    add_target_group_client_.reset();
    add_cluster_group_client_.reset();
    remove_all_targets_client_.reset();
    remove_all_clusters_client_.reset();
    // Destroy publishers
    update_target_cmd_group_pub_.reset();
    update_cluster_cmd_group_pub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// GLOBAL CONTROL: Service-based control methods
// ════════════════════════════════════════════════════════════════════════════

bool SimulationBridge::resetSimulation()
{
    // Create request
    auto request = std::make_shared<ResetSrv::Request>();

    // Send request and wait for response
    return node_->sendRequest<ResetSrv>(reset_client_, request, 1000);
}

bool SimulationBridge::runSimulation()
{
    // Create request
    auto request = std::make_shared<RunSrv::Request>();

    // Send request and wait for response
    return node_->sendRequest<RunSrv>(run_client_, request, 1000);
}

bool SimulationBridge::pauseSimulation()
{
    // Create request
    auto request = std::make_shared<PauseSrv::Request>();

    // Send request and wait for response
    return node_->sendRequest<PauseSrv>(pause_client_, request, 1000);
}

// ════════════════════════════════════════════════════════════════════════════
// TRACKING CONTROL: Service-based control methods
// ════════════════════════════════════════════════════════════════════════════

bool SimulationBridge::addTargetGroup(const IDs& target_ids, const std::vector<TargetType>& target_types, const std::vector<PointMsg>& positions, const bool& highlight, const std::vector<ColorMsg>& highlight_colors)
{
    // Create request
    auto request = std::make_shared<AddTargetGroupSrv::Request>();
    request->target_names = target_ids;
    request->positions = positions;
    request->highlight = highlight;
    request->highlight_color_rgba = highlight_colors;

    for (size_t i = 0; i < target_ids.size(); ++i)
    {
        // Set target type based on target type
        switch (target_types[i])
        {
        case TargetType::Human:
            request->target_types.push_back("Human");
            break;
        default:
            RCLCPP_ERROR(node_->get_logger(), "Unknown target type: %d", static_cast<int>(target_types[i]));
            request->target_types.push_back("Cube");
            break;
        }
    }

    // Send request and wait for response
    return node_->sendRequest<AddTargetGroupSrv>(add_target_group_client_, request, 100000);
}

bool SimulationBridge::addClusterGroup(const IDs& cluster_ids, const std::vector<PointMsg>& centers, const std::vector<float>& radii, const bool& highlight, const std::vector<ColorMsg>& highlight_colors)
{
    // Create request
    auto request = std::make_shared<AddClusterGroupSrv::Request>();
    request->cluster_names = cluster_ids;
    request->centers = centers;
    request->radii = radii;
    request->highlight = highlight;
    request->highlight_color_rgba = highlight_colors;

    // Send request and wait for response
    return node_->sendRequest<AddClusterGroupSrv>(add_cluster_group_client_, request, 100000);
}

bool SimulationBridge::removeAllTargets()
{
    // Create request
    auto request = std::make_shared<RemoveAllTargetsSrv::Request>();

    // Send request and wait for response
    return node_->sendRequest<RemoveAllTargetsSrv>(remove_all_targets_client_, request, 100000);
}

bool SimulationBridge::removeAllClusters()
{
    // Create request
    auto request = std::make_shared<RemoveAllClustersSrv::Request>();

    // Send request and wait for response
    return node_->sendRequest<RemoveAllClustersSrv>(remove_all_clusters_client_, request, 100000);
}

// ════════════════════════════════════════════════════════════════════════════
// OBJECT CONTROL: Publisher-based control methods
// ════════════════════════════════════════════════════════════════════════════

void SimulationBridge::updateTargetGroup(const IDs& target_ids, const std::vector<PointMsg>& positions)
{
    // Create message
    UpdateTargetCmdGroupMsg msg;
    msg.target_names = target_ids;
    msg.positions = positions;

    // Publish message
    update_target_cmd_group_pub_->publish(msg);
}

void SimulationBridge::updateClusterGroup(const IDs& cluster_ids, const std::vector<PointMsg>& centers, const std::vector<float>& radii)
{
    // Create message
    UpdateClusterCmdGroupMsg msg;
    msg.cluster_names = cluster_ids;
    msg.centers = centers;
    msg.radii = radii;

    // Publish message
    update_cluster_cmd_group_pub_->publish(msg);
}