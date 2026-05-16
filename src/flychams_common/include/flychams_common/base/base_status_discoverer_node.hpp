#pragma once

// Base node include
#include "flychams_common/base/base_status_node.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base node for discovering system elements
     *
     * @details
     * Extends BaseStatusNode with element (agent, target, cluster)
     * discovery via a RegistrationMsg subscriber. Provides onAdd/onRemove
     * callbacks for dynamic element management. Used as the base for
     * multi-element coordinator and simulation nodes.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class BaseStatusDiscovererNode : public BaseStatusNode
    {
    public: // Constructor/Destructor
        BaseStatusDiscovererNode(const std::string& node_name, const rclcpp::NodeOptions& options);

        virtual ~BaseStatusDiscovererNode() = default;

    public: // Types
        using SharedPtr = std::shared_ptr<BaseStatusDiscovererNode>;

    protected: // Overridable discovery hooks
        virtual void onDiscoveryInit() {}
        virtual void onDiscoveryShutdown() {}

    protected: // Overridable discovery callbacks
        virtual void onAddAgent(const ID& agent_id) {}
        virtual void onRemoveAgent(const ID& agent_id) {}
        virtual void onAddTarget(const ID& target_id) {}
        virtual void onRemoveTarget(const ID& target_id) {}
        virtual void onAddCluster(const ID& cluster_id) {}
        virtual void onRemoveCluster(const ID& cluster_id) {}

    protected: // Overridable methods
        void onStatusInit() override;
        void onStatusShutdown() override;

    public: // Shared from this
        SharedPtr sharedFromThis()
        {
            return std::dynamic_pointer_cast<BaseStatusDiscovererNode>(shared_from_this());
        }

    private: // Discovery data
        std::unordered_map<ID, ElementType> elements_;

    private: // ROS components
        // Subscribers
        SubscriberPtr<RegistrationMsg> discovery_sub_;

    private: // Discovery callback
        void onDiscovery(const RegistrationMsg::SharedPtr msg);
    };

} // namespace flychams::common