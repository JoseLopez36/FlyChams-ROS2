#pragma once

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base node with mission and fleet status subscriptions
     *
     * @details
     * Extends BaseNode with subscriptions to MissionStatus and
     * FleetStatus topics. Provides query methods for mission/fleet
     * state, equivalent to LimitedModule but at the node level.
     * Used as the base for agent-level nodes.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-15
     * ════════════════════════════════════════════════════════════════
     */
    class BaseStatusNode : public BaseNode
    {
    public: // Constructor/Destructor
        BaseStatusNode(const std::string& node_name, const rclcpp::NodeOptions& options);

        virtual ~BaseStatusNode() = default;

    public: // Types
        using SharedPtr = std::shared_ptr<BaseStatusNode>;

    protected: // Overridable methods
        void onNodeInit() override;
        void onNodeShutdown() override;

    protected: // Overridable status hooks
        virtual void onStatusInit() {}
        virtual void onStatusShutdown() {}

    public: // Shared from this
        SharedPtr sharedFromThis()
        {
            return std::dynamic_pointer_cast<BaseStatusNode>(shared_from_this());
        }

    private: // Status data
        MissionStatus mission_status_ = MissionStatus::READY;
        FleetStatus fleet_status_ = FleetStatus::IDLE;
        bool has_mission_status_ = false;
        bool has_fleet_status_ = false;

    private: // ROS components
        // Subscribers
        SubscriberPtr<MissionStatusMsg> mission_status_sub_;
        SubscriberPtr<FleetStatusMsg> fleet_status_sub_;

    private: // Status callbacks
        void missionStatusCallback(const MissionStatusMsg::SharedPtr msg);
        void fleetStatusCallback(const FleetStatusMsg::SharedPtr msg);

    public: // Status getters
        MissionStatus getMissionStatus() const { return mission_status_; };
        FleetStatus getFleetStatus() const { return fleet_status_; };

    public: // Status queries
        bool isMissionReady() const;
        bool isMissionActive() const;
        bool isMissionPaused() const;
        bool isMissionAborted() const;
        bool isFleetIdle() const;
        bool isFleetActive() const;
        bool isFleetMixed() const;
        bool isFleetError() const;
    };

} // namespace flychams::common