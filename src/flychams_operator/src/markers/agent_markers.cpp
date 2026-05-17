#include "flychams_operator/markers/agent_markers.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void AgentMarkers::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Initialize data
    agent_ = AgentData();

    // Publishers
    agent_.markers_pub = node_->createAgentMarkersPublisher(agent_id_);

    // Subscribers
    agent_.local_position_sub = node_->createAgentLocalPositionSubscriber(agent_id_,
        std::bind(&AgentMarkers::localPositionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    agent_.status_sub = node_->createAgentStatusSubscriber(agent_id_,
        std::bind(&AgentMarkers::statusCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentMarkers::update, this));
}

void AgentMarkers::onModuleShutdown()
{
    agent_.markers_pub.reset();
    agent_.local_position_sub.reset();
    agent_.status_sub.reset();
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void AgentMarkers::localPositionCallback(const PointStampedMsg::SharedPtr msg)
{
    agent_.position = msg->point;
    agent_.has_position = true;
}

void AgentMarkers::statusCallback(const AgentStatusMsg::SharedPtr msg)
{
    agent_.status = msg->status;
    agent_.has_status = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void AgentMarkers::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Agent markers: Skipping update due to invalid status");
        return;
    }

    const std::string& frame = node_->getGlobalFrame();
    auto stamp = node_->now();

    MarkerArrayMsg array;

    // ── Sphere marker representing the drone body ──────────────────────────
    MarkerMsg body;
    body.header.frame_id = frame;
    body.header.stamp = stamp;
    body.ns = agent_id_;
    body.id = 0;
    body.type = MarkerMsg::SPHERE;
    body.action = MarkerMsg::ADD;
    body.pose.position = agent_.position;
    body.pose.orientation.w = 1.0;
    body.scale.x = 1.0;
    body.scale.y = 1.0;
    body.scale.z = 0.4;
    // Color: blue when tracking, yellow otherwise
    if (agent_.has_status && agent_.status == 7 /* TRACKING */)
    {
        body.color.r = 0.0f; body.color.g = 0.5f; body.color.b = 1.0f; body.color.a = 1.0f;
    }
    else
    {
        body.color.r = 1.0f; body.color.g = 0.8f; body.color.b = 0.0f; body.color.a = 1.0f;
    }
    array.markers.push_back(body);

    // ── Text label ─────────────────────────────────────────────────────────
    MarkerMsg label;
    label.header.frame_id = frame;
    label.header.stamp = stamp;
    label.ns = agent_id_ + "_label";
    label.id = 1;
    label.type = MarkerMsg::TEXT_VIEW_FACING;
    label.action = MarkerMsg::ADD;
    label.pose.position = agent_.position;
    label.pose.position.z += 1.2;
    label.pose.orientation.w = 1.0;
    label.scale.z = 0.8;
    label.color.r = 1.0f; label.color.g = 1.0f; label.color.b = 1.0f; label.color.a = 1.0f;
    label.text = agent_id_;
    array.markers.push_back(label);

    agent_.markers_pub->publish(array);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool AgentMarkers::checkStatus()
{
    // Check 1: Agent must have a valid position
    if (!agent_.has_position)
    {
        RCLCPP_WARN(node_->get_logger(), "Agent markers: Agent %s has no position", agent_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}