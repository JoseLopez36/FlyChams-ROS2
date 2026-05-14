#include "flychams_simulation/tools/airsim_tools.hpp"

using namespace airsim_interfaces::srv;
using namespace flychams::core;

namespace flychams::simulation
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    AirsimTools::AirsimTools(NodePtr node, const SettingsTools::SharedPtr& settings_tools)
        : SimulationTools(node, settings_tools)
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

    AirsimTools::~AirsimTools()
    {
        shutdown();
    }

    void AirsimTools::shutdown()
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
        // Destroy node pointer
        node_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // GLOBAL CONTROL: Service-based control methods
    // ════════════════════════════════════════════════════════════════════════════

    bool AirsimTools::resetSimulation()
    {
        // Create request
        auto request = std::make_shared<Reset::Request>();

        // Send request and wait for response
        return RosUtils::sendRequest<Reset>(node_, reset_client_, request, 1000);
    }

    bool AirsimTools::runSimulation()
    {
        // Create request
        auto request = std::make_shared<Run::Request>();

        // Send request and wait for response
        return RosUtils::sendRequest<Run>(node_, run_client_, request, 1000);
    }

    bool AirsimTools::pauseSimulation()
    {
        // Create request
        auto request = std::make_shared<Pause::Request>();

        // Send request and wait for response
        return RosUtils::sendRequest<Pause>(node_, pause_client_, request, 1000);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // TRACKING CONTROL: Service-based control methods
    // ════════════════════════════════════════════════════════════════════════════

    bool AirsimTools::addTargetGroup(const IDs& target_ids, const std::vector<TargetType>& target_types, const std::vector<PointMsg>& positions, const bool& highlight, const std::vector<ColorMsg>& highlight_colors)
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
        return RosUtils::sendRequest<AddTargetGroup>(node_, add_target_group_client_, request, 100000);
    }

    bool AirsimTools::addClusterGroup(const IDs& cluster_ids, const std::vector<PointMsg>& centers, const std::vector<float>& radii, const bool& highlight, const std::vector<ColorMsg>& highlight_colors)
    {
        // Create request
        auto request = std::make_shared<AddClusterGroup::Request>();
        request->cluster_names = cluster_ids;
        request->centers = centers;
        request->radii = radii;
        request->highlight = highlight;
        request->highlight_color_rgba = highlight_colors;

        // Send request and wait for response
        return RosUtils::sendRequest<AddClusterGroup>(node_, add_cluster_group_client_, request, 100000);
    }

    bool AirsimTools::removeAllTargets()
    {
        // Create request
        auto request = std::make_shared<RemoveAllTargets::Request>();

        // Send request and wait for response
        return RosUtils::sendRequest<RemoveAllTargets>(node_, remove_all_targets_client_, request, 100000);
    }

    bool AirsimTools::removeAllClusters()
    {
        // Create request
        auto request = std::make_shared<RemoveAllClusters::Request>();

        // Send request and wait for response
        return RosUtils::sendRequest<RemoveAllClusters>(node_, remove_all_clusters_client_, request, 100000);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // OBJECT CONTROL: Publisher-based control methods
    // ════════════════════════════════════════════════════════════════════════════

    void AirsimTools::updateTargetGroup(const IDs& target_ids, const std::vector<PointMsg>& positions)
    {
        // Create message
        UpdateTargetCmdGroup msg;
        msg.target_names = target_ids;
        msg.positions = positions;

        // Publish message
        update_target_cmd_group_pub_->publish(msg);
    }

    void AirsimTools::updateClusterGroup(const IDs& cluster_ids, const std::vector<PointMsg>& centers, const std::vector<float>& radii)
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