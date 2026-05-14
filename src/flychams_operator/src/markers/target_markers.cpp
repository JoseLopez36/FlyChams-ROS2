#include "flychams_operator/markers/target_markers.hpp"

#include "flychams_common/utils/ros_utils.hpp"

using namespace flychams::core;

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // INIT / SHUTDOWN
    // ════════════════════════════════════════════════════════════════════════════

    void TargetMarkers::onInit()
    {
        // Get parameters
        update_rate_ = RosUtils::getParameterOr<float>(node_, "update_rate", 10.0f);

        // Initialize data
        target_ = TargetData();

        // Publishers
        target_.markers_pub = topic_tools_->createTargetMarkersPublisher(target_id_);

        // Subscribers
        target_.position_sub = topic_tools_->createTargetPositionSubscriber(target_id_,
            std::bind(&TargetMarkers::positionCallback, this, std::placeholders::_1),
            sub_options_with_module_cb_group_);

        // Update timer
        update_timer_ = rclcpp::create_timer(node_,
            node_->get_clock(),
            std::chrono::duration<float>(1.0f / update_rate_),
            std::bind(&TargetMarkers::update, this),
            module_cb_group_);
    }

    void TargetMarkers::onShutdown()
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
        if (!target_.has_position)
        {
            return;
        }

        const std::string& frame = transform_tools_->getGlobalFrame();
        auto stamp = RosUtils::now(node_);

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

} // namespace flychams::operator_pkg