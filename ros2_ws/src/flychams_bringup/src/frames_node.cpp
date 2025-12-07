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
        
        // Global origin publisher
        global_origin_pub_ = node_->create_publisher<GeoPointStampedMsg>(
            "/flychams/global_origin", rclcpp::QoS(10).transient_local());
    }

    void onShutdown() override
    {
        // Destroy frames managers
        frames_managers_.clear();
        global_origin_pub_.reset();
        first_agent_gps_sub_.reset();
    }

private: // Agent management
    void onAddAgent(const ID& agent_id) override
    {
        // Check if global origin is set
        if (!global_origin_set_ && !first_agent_gps_sub_)
        {
            RCLCPP_INFO(node_->get_logger(), "Subscribing to GPS of first agent %s to set global origin", agent_id.c_str());
            first_agent_gps_sub_ = node_->create_subscription<NavSatFixMsg>(
                "/mavros/" + agent_id + "/global_position/global",
                rclcpp::SensorDataQoS(),
                std::bind(&FramesNode::gpsCallback, this, std::placeholders::_1)
            );
        }

        // Use callback group from discovery node (to avoid race conditions)
        // Create and add frames manager
        auto manager = std::make_shared<FramesManager>(agent_id, node_, config_tools_, topic_tools_, transform_tools_, discovery_cb_group_);
        frames_managers_.insert({ agent_id, manager });

        RCLCPP_INFO(node_->get_logger(), "Frames manager created for agent %s", agent_id.c_str());
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Remove agent from frames managers
        frames_managers_.erase(agent_id);
    }

    void gpsCallback(const sensor_msgs::msg::NavSatFix::SharedPtr msg)
    {
        if (global_origin_set_) return;

        RCLCPP_INFO(node_->get_logger(), "Global origin set from first agent: %f, %f, %f", msg->latitude, msg->longitude, msg->altitude);
        
        GeoPointStampedMsg origin_msg;
        origin_msg.header.stamp = node_->now();
        origin_msg.header.frame_id = "world";
        origin_msg.position.latitude = msg->latitude;
        origin_msg.position.longitude = msg->longitude;
        origin_msg.position.altitude = msg->altitude;

        global_origin_pub_->publish(origin_msg);
        global_origin_set_ = true;
        
        // Unsubscribe
        first_agent_gps_sub_.reset();
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
    
    // Global origin
    bool global_origin_set_ = false;
    SubscriberPtr<NavSatFixMsg> first_agent_gps_sub_;
    PublisherPtr<GeoPointStampedMsg> global_origin_pub_;
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

