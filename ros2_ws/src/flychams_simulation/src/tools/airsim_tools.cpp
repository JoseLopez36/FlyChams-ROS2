#include "flychams_simulation/tools/airsim_tools.hpp"

using namespace airsim_interfaces::msg;
using namespace airsim_interfaces::srv;
using namespace flychams::core;

namespace flychams::simulation
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    AirsimTools::AirsimTools(NodePtr node, const ConfigTools::SharedPtr& config_tools)
        : SimulationTools(node, config_tools)
    {
        // Initialize ROS components
        // Global commands
        reset_client_ = node_->create_client<Reset>("/airsim/reset");
        run_client_ = node_->create_client<Run>("/airsim/run");
        pause_client_ = node_->create_client<Pause>("/airsim/pause");
        // Window commands
        window_image_cmd_group_pub_ = node_->create_publisher<WindowImageCmdGroup>("/airsim/windows/cmd/image", 10);
        window_rectangle_cmd_pub_ = node_->create_publisher<WindowRectangleCmd>("/airsim/windows/cmd/rectangle", 10);
        window_string_cmd_pub_ = node_->create_publisher<WindowStringCmd>("/airsim/windows/cmd/string", 10);
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
        window_image_cmd_group_pub_.reset();
        window_rectangle_cmd_pub_.reset();
        window_string_cmd_pub_.reset();
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
    // WINDOW CONTROL: Service-based control methods
    // ════════════════════════════════════════════════════════════════════════════

    void AirsimTools::setWindows(const std::vector<WindowCmd>& window_cmds)
    {
        // Get number of windows
        const size_t n = window_cmds.size();

        // Create message
        WindowImageCmdGroup msg;
        msg.window_indices.resize(n);
        msg.vehicle_names.resize(n);
        msg.camera_names.resize(n);
        msg.corners.resize(n);
        msg.sizes.resize(n);
        for (size_t i = 0; i < n; i++)
        {
            const auto& cmd = window_cmds[i];

            msg.window_indices[i] = getWindowIndex(cmd.window_id);
            msg.vehicle_names[i] = cmd.vehicle_id;
            msg.camera_names[i] = cmd.camera_id;
            msg.corners[i].x = cmd.crop.x;
            msg.corners[i].y = cmd.crop.y;
            msg.sizes[i].x = cmd.crop.w;
            msg.sizes[i].y = cmd.crop.h;
        }

        // Publish message
        window_image_cmd_group_pub_->publish(msg);
    }

    void AirsimTools::drawWindow(const DrawCmd& draw_cmd)
    {
        // Get number of elements to draw
        const size_t n_rectangles = draw_cmd.rectangles.positions.size();
        const size_t n_strings = draw_cmd.strings.positions.size();

        // Create rectangle message
        WindowRectangleCmd rectangle_msg;
        rectangle_msg.window_index = getWindowIndex(draw_cmd.window_id);
        rectangle_msg.corners = draw_cmd.rectangles.positions;
        rectangle_msg.sizes = draw_cmd.rectangles.sizes;
        rectangle_msg.color = draw_cmd.rectangles.color;
        rectangle_msg.thickness = draw_cmd.rectangles.thickness;

        // Create string message
        WindowStringCmd string_msg;
        string_msg.window_index = getWindowIndex(draw_cmd.window_id);
        string_msg.strings = draw_cmd.strings.texts;
        string_msg.positions = draw_cmd.strings.positions;
        string_msg.color = draw_cmd.strings.color;
        string_msg.scale = draw_cmd.strings.scale;

        // Publish rectangle and string messages
        window_rectangle_cmd_pub_->publish(rectangle_msg);
        window_string_cmd_pub_->publish(string_msg);
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
            case TargetType::Cube:
                request->target_types.push_back("Cube");
                break;
            case TargetType::Human:
                request->target_types.push_back("Human");
                break;
            case TargetType::MetaHuman:
                request->target_types.push_back("MetaHuman");
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