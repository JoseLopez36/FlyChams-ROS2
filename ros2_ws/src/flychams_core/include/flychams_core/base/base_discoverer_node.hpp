#pragma once

// Tools includes
#include "flychams_core/settings/settings_tools.hpp"
#include "flychams_core/ros/topic_tools.hpp"
#include "flychams_core/ros/transform_tools.hpp"

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/ros_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Discoverer node for discovering the different elements
     * in the simulation
     *
     * @details
     * This class implements the discoverer node for discovering agents,
     * targets, and clusters with the help of the various tools. It serves
     * as a base class for the different nodes that need to discover elements
     * dynamically in the simulation.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class BaseDiscovererNode : public rclcpp::Node
    {
    public: // Constructor/Destructor
        BaseDiscovererNode(const std::string& node_name, const rclcpp::NodeOptions& options);
        
        void init();

        virtual ~BaseDiscovererNode();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseDiscovererNode>;

    protected: // Overridable methods
        virtual void onInit() {}
        virtual void onShutdown() {}
        virtual void onAddAgent(const ID& agent_id) {}
        virtual void onRemoveAgent(const ID& agent_id) {}
        virtual void onAddTarget(const ID& target_id) {}
        virtual void onRemoveTarget(const ID& target_id) {}
        virtual void onAddCluster(const ID& cluster_id) {}
        virtual void onRemoveCluster(const ID& cluster_id) {}

    private: // Discovery callback
        void onDiscovery(const RegistrationMsg::SharedPtr msg);

    protected: // Components
        // Node
        NodePtr node_;
        const std::string node_name_;
        // Tools
        SettingsTools::SharedPtr config_tools_;
        TopicTools::SharedPtr topic_tools_;
        TransformTools::SharedPtr transform_tools_;
        // Discovered elements
        std::unordered_map<ID, ElementType> elements_;
        // Callback group
        CallbackGroupPtr discovery_cb_group_;
        rclcpp::SubscriptionOptions sub_options_with_discovery_cb_group_;
        // Discovery subscriber
        SubscriberPtr<RegistrationMsg> discovery_sub_;
    };

} // namespace flychams::core