#include "flychams_common/base/base_status_node.hpp"

namespace flychams::core
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    BaseStatusNode::BaseStatusNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNode(node_name, options)
    {
        // Nothing to do
    }

    void BaseStatusNode::onNodeInit()
    {
        // Subscribe to mission and fleet status topics
        mission_status_sub_ = createMissionStatusSubscriber(
            [this](const MissionStatusMsg::SharedPtr msg) { missionStatusCallback(msg); },
            sub_options_with_node_cb_group_);

        fleet_status_sub_ = createFleetStatusSubscriber(
            [this](const FleetStatusMsg::SharedPtr msg) { fleetStatusCallback(msg); },
            sub_options_with_node_cb_group_);

        // Call on status init overridable method
        onStatusInit();
    }

    void BaseStatusNode::onNodeShutdown()
    {
        // Call on status shutdown overridable method
        onStatusShutdown();
        // Destroy status subscribers
        mission_status_sub_.reset();
        fleet_status_sub_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATUS CALLBACKS
    // ════════════════════════════════════════════════════════════════════════════

    void BaseStatusNode::missionStatusCallback(const MissionStatusMsg::SharedPtr msg)
    {
        mission_status_ = static_cast<MissionStatus>(msg->status);
        has_mission_status_ = true;
    }

    void BaseStatusNode::fleetStatusCallback(const FleetStatusMsg::SharedPtr msg)
    {
        fleet_status_ = static_cast<FleetStatus>(msg->status);
        has_fleet_status_ = true;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATUS QUERIES
    // ════════════════════════════════════════════════════════════════════════════

    bool BaseStatusNode::isMissionActive() const
    {
        if (!has_mission_status_) return false;
        return mission_status_ == MissionStatus::ACTIVE;
    }

    bool BaseStatusNode::isMissionPaused() const
    {
        if (!has_mission_status_) return false;
        return mission_status_ == MissionStatus::PAUSED;
    }

    bool BaseStatusNode::isMissionAborted() const
    {
        if (!has_mission_status_) return false;
        return mission_status_ == MissionStatus::ABORTED;
    }

} // namespace flychams::core