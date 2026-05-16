#include "flychams_simulation/bridge/simulation_bridge.hpp"

using namespace airsim_interfaces::msg;
using namespace airsim_interfaces::srv;
using namespace flychams::core;

namespace flychams::simulation
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void SimulationBridge::onModuleInit()
    {
        // Initialize ROS components
        // Global commands
        reset_client_ = node_->create_client<Reset>("/airsim/reset");
        run_client_ = node_->create_client<Run>("/airsim/run");
        pause_client_ = node_->create_client<Pause>("/airsim/pause");
        // Tracking commands
        add_target_group_client_ = node_->create_client<AddTargetGroup>("/airsim/targets/cmd/add");
        add_cluster_group_client_ = node_->create_client<AddClusterGroup>("/airsim/clusters/cmd/add");
        remove_all_targets_client_ = node_->create_client<RemoveAllTargets>("/airsim/targets/cmd/remove_all");
        remove_all_clusters_client_ = node_->create_client<RemoveAllClusters>("/airsim/clusters/cmd/remove_all");
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
        auto request = std::make_shared<Reset::Request>();

        // Send request and wait for response
        return node_->sendRequest<Reset>(reset_client_, request, 1000);
    }

    bool SimulationBridge::runSimulation()
    {
        // Create request
        auto request = std::make_shared<Run::Request>();

        // Send request and wait for response
        return node_->sendRequest<Run>(run_client_, request, 1000);
    }

    bool SimulationBridge::pauseSimulation()
    {
        // Create request
        auto request = std::make_shared<Pause::Request>();

        // Send request and wait for response
        return node_->sendRequest<Pause>(pause_client_, request, 1000);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // TRACKING CONTROL: Service-based control methods
    // ════════════════════════════════════════════════════════════════════════════

    bool SimulationBridge::addTargetGroup(const IDs& target_ids, const std::vector<TargetType>& target_types, const std::vector<PointMsg>& positions, const bool& highlight, const std::vector<ColorMsg>& highlight_colors)
    {
        // Create request
        auto request = std::make_shared<AddTargetGroup::Request>();
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
        return node_->sendRequest<AddTargetGroup>(add_target_group_client_, request, 100000);
    }

    bool SimulationBridge::addClusterGroup(const IDs& cluster_ids, const std::vector<PointMsg>& centers, const std::vector<float>& radii, const bool& highlight, const std::vector<ColorMsg>& highlight_colors)
    {
        // Create request
        auto request = std::make_shared<AddClusterGroup::Request>();
        request->cluster_names = cluster_ids;
        request->centers = centers;
        request->radii = radii;
        request->highlight = highlight;
        request->highlight_color_rgba = highlight_colors;

        // Send request and wait for response
        return node_->sendRequest<AddClusterGroup>(add_cluster_group_client_, request, 100000);
    }

    bool SimulationBridge::removeAllTargets()
    {
        // Create request
        auto request = std::make_shared<RemoveAllTargets::Request>();

        // Send request and wait for response
        return node_->sendRequest<RemoveAllTargets>(remove_all_targets_client_, request, 100000);
    }

    bool SimulationBridge::removeAllClusters()
    {
        // Create request
        auto request = std::make_shared<RemoveAllClusters::Request>();

        // Send request and wait for response
        return node_->sendRequest<RemoveAllClusters>(remove_all_clusters_client_, request, 100000);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // OBJECT CONTROL: Publisher-based control methods
    // ════════════════════════════════════════════════════════════════════════════

    void SimulationBridge::updateTargetGroup(const IDs& target_ids, const std::vector<PointMsg>& positions)
    {
        // Create message
        UpdateTargetCmdGroup msg;
        msg.target_names = target_ids;
        msg.positions = positions;

        // Publish message
        update_target_cmd_group_pub_->publish(msg);
    }

    void SimulationBridge::updateClusterGroup(const IDs& cluster_ids, const std::vector<PointMsg>& centers, const std::vector<float>& radii)
    {
        // Create message
        UpdateClusterCmdGroup msg;
        msg.cluster_names = cluster_ids;
        msg.centers = centers;
        msg.radii = radii;

        // Publish message
        update_cluster_cmd_group_pub_->publish(msg);
    }

} // namespace flychams::simulation