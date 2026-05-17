#include "flychams_operator/markers/target_markers.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void TargetMarkers::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Initialize data
    target_ = TargetData();

    // Publishers
    target_.markers_pub = node_->createTargetMarkersPublisher(target_id_);

    // Subscribers
    target_.position_sub = node_->createTargetPositionSubscriber(target_id_,
        std::bind(&TargetMarkers::positionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&TargetMarkers::update, this));
}

void TargetMarkers::onModuleShutdown()
{
    target_.markers_pub.reset();
    target_.position_sub.reset();
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void TargetMarkers::positionCallback(const PointStampedMsg::SharedPtr msg)
{
    target_.position = msg->point;
    target_.has_position = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void TargetMarkers::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Target markers: Skipping update due to invalid status");
        return;
    }

    const std::string& frame = node_->getGlobalFrame();
    auto stamp = node_->now();

    MarkerArrayMsg array;

    // ── Cylinder marker representing the target body ───────────────────────
    MarkerMsg body;
    body.header.frame_id = frame;
    body.header.stamp = stamp;
    body.ns = target_id_;
    body.id = 0;
    body.type = MarkerMsg::CYLINDER;
    body.action = MarkerMsg::ADD;
    body.pose.position = target_.position;
    body.pose.orientation.w = 1.0;
    body.scale.x = 0.5;
    body.scale.y = 0.5;
    body.scale.z = 1.8;
    body.color.r = 1.0f; body.color.g = 0.2f; body.color.b = 0.2f; body.color.a = 1.0f;
    array.markers.push_back(body);

    // ── Text label ─────────────────────────────────────────────────────────
    MarkerMsg label;
    label.header.frame_id = frame;
    label.header.stamp = stamp;
    label.ns = target_id_ + "_label";
    label.id = 1;
    label.type = MarkerMsg::TEXT_VIEW_FACING;
    label.action = MarkerMsg::ADD;
    label.pose.position = target_.position;
    label.pose.position.z += 2.2;
    label.pose.orientation.w = 1.0;
    label.scale.z = 0.6;
    label.color.r = 1.0f; label.color.g = 1.0f; label.color.b = 1.0f; label.color.a = 1.0f;
    label.text = target_id_;
    array.markers.push_back(label);

    target_.markers_pub->publish(array);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool TargetMarkers::checkStatus()
{
    // Check 1: Target must have a valid position
    if (!target_.has_position)
    {
        return false;
    }

    // All checks passed
    return true;
}