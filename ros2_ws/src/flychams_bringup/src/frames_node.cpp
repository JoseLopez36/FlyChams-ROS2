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
    }

private: // Agent management
    void onAddAgent(const ID& agent_id) override
    {
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

