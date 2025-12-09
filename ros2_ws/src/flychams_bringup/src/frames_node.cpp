#include "rclcpp/rclcpp.hpp"

// Bringup includes
#include "flychams_bringup/frames/frames_manager.hpp"

// Core includes
#include "flychams_core/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::bringup;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Frames Node for managing transforms in the simulation
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-02-28
 * ════════════════════════════════════════════════════════════════
 */
class FramesNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    FramesNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Initialize frames managers
        frames_managers_.clear();

        // Create global origin publisher
        global_origin_pub_ = topic_tools_->createGlobalOriginPublisher();

        // Publish global origin
        GeoPointStampedMsg origin_msg;
        origin_msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        origin_msg.position.latitude = config_tools_->getEnvironment().geopoint.latitude;
        origin_msg.position.longitude = config_tools_->getEnvironment().geopoint.longitude;
        origin_msg.position.altitude = config_tools_->getEnvironment().geopoint.altitude;
        global_origin_pub_->publish(origin_msg);
    }

    void onShutdown() override
    {
        // Destroy frames managers
        frames_managers_.clear();
        global_origin_pub_.reset();
        // Clear navsat clients
        navsat_clients_.clear();
    }

private: // Agent management
    void onAddAgent(const ID& agent_id) override
    {
        // Use callback group from discovery node (to avoid race conditions)
        // Create and add frames manager
        auto manager = std::make_shared<FramesManager>(agent_id, node_, config_tools_, topic_tools_, transform_tools_, discovery_cb_group_);
        frames_managers_.insert({ agent_id, manager });

        RCLCPP_INFO(node_->get_logger(), "Frames manager created for agent %s", agent_id.c_str());

        // Set datum for navsat_transform_node via service call
        setNavsatDatum(agent_id);
    }

    void setNavsatDatum(const ID& agent_id)
    {
        // Create service client for navsat_transform_node set_datum service
        std::string service_name = "/navsat/" + agent_id + "/datum";
        auto client = node_->create_client<robot_localization::srv::SetDatum>(service_name);

        // Wait for service to be available (with timeout)
        if (!client->wait_for_service(std::chrono::seconds(60)))
        {
            RCLCPP_WARN(node_->get_logger(), "Service %s not available, datum will be set when service becomes available", service_name.c_str());
            // Store client for later retry
            navsat_clients_[agent_id] = client;
            return;
        }

        RCLCPP_INFO(node_->get_logger(), "Service %s available, setting datum for agent %s", service_name.c_str(), agent_id.c_str());

        // Get environment geopoint
        const auto& env = config_tools_->getEnvironment();

        // Create service request
        auto request = std::make_shared<robot_localization::srv::SetDatum::Request>();
        request->geo_pose.position.latitude = env.geopoint.latitude;
        request->geo_pose.position.longitude = env.geopoint.longitude;
        request->geo_pose.position.altitude = env.geopoint.altitude;
        // Set orientation to identity (no rotation)
        request->geo_pose.orientation.x = 0.0;
        request->geo_pose.orientation.y = 0.0;
        request->geo_pose.orientation.z = 0.0;
        request->geo_pose.orientation.w = 1.0;

        // Send async request
        auto future = client->async_send_request(request);

        // Store client for potential retries
        navsat_clients_[agent_id] = client;

        RCLCPP_INFO(node_->get_logger(), "Set datum for agent %s navsat_transform_node: lat=%.6f, lon=%.6f, alt=%.3f",
            agent_id.c_str(), env.geopoint.latitude, env.geopoint.longitude, env.geopoint.altitude);
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Remove agent from frames managers
        frames_managers_.erase(agent_id);
        // Remove navsat client
        navsat_clients_.erase(agent_id);
    }

    void onAddTarget(const ID& target_id) override
    {
        // Targets are not handled by this node
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Targets are not handled by this node
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Clusters are not handled by this node
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Clusters are not handled by this node
    }

private: // Components
    // Frames manager per agent
    std::unordered_map<ID, FramesManager::SharedPtr> frames_managers_;

    // Global origin publisher
    PublisherPtr<GeoPointStampedMsg> global_origin_pub_;

    // Navsat transform service clients (for setting datum)
    std::unordered_map<ID, rclcpp::Client<robot_localization::srv::SetDatum>::SharedPtr> navsat_clients_;
};

int main(int argc, char** argv)
{
    // Initialize ROS
    rclcpp::init(argc, argv);
    // Create node options
    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(true);
    // Create and initialize node
    auto node = std::make_shared<FramesNode>("frames_node", options);
    node->init();
    // Spin node
    rclcpp::spin(node);
    // Shutdown
    rclcpp::shutdown();
    return 0;
}

