#include "flychams_operator/metrics/target_metrics.hpp"

#include "flychams_common/utils/ros_utils.hpp"

using namespace flychams::core;

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // INIT / SHUTDOWN
    // ════════════════════════════════════════════════════════════════════════════

    void TargetMetrics::onInit()
    {
        // Get parameters
        update_rate_ = RosUtils::getParameterOr<float>(node_, "update_rate", 10.0f);

        // Initialize data
        target_ = TargetData();
        distance_traveled_ = 0.0f;
        total_speed_ = 0.0f;
        speed_samples_ = 0;
        time_elapsed_ = 0.0f;
        last_update_time_ = RosUtils::now(node_);

        // Publishers
        target_.metrics_pub = topic_tools_->createTargetMetricsPublisher(target_id_);

        // Subscribers
        target_.position_sub = topic_tools_->createTargetPositionSubscriber(target_id_,
            std::bind(&TargetMetrics::positionCallback, this, std::placeholders::_1),
            sub_options_with_module_cb_group_);

        // Update timer
        update_timer_ = rclcpp::create_timer(node_,
            node_->get_clock(),
            std::chrono::duration<float>(1.0f / update_rate_),
            std::bind(&TargetMetrics::update, this),
            module_cb_group_);
    }

    void TargetMetrics::onShutdown()
    {
        target_.metrics_pub.reset();
        target_.position_sub.reset();
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS
    // ════════════════════════════════════════════════════════════════════════════

    void TargetMetrics::positionCallback(const PointStampedMsg::SharedPtr msg)
    {
        if (target_.has_position)
        {
            float dx = msg->point.x - target_.position.x;
            float dy = msg->point.y - target_.position.y;
            float dz = msg->point.z - target_.position.z;
            distance_traveled_ += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        else
        {
            last_position_ = msg->point;
        }
        target_.position = msg->point;
        target_.has_position = true;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE
    // ════════════════════════════════════════════════════════════════════════════

    void TargetMetrics::update()
    {
        if (!target_.has_position)
        {
            return;
        }

        // Compute dt
        auto now = RosUtils::now(node_);
        float dt = static_cast<float>((now - last_update_time_).seconds());
        last_update_time_ = now;
        time_elapsed_ += dt;

        // Compute instantaneous speed
        float dx = target_.position.x - last_position_.x;
        float dy = target_.position.y - last_position_.y;
        float dz = target_.position.z - last_position_.z;
        float speed = (dt > 0.0f) ? std::sqrt(dx * dx + dy * dy + dz * dz) / dt : 0.0f;
        last_position_ = target_.position;

        // Accumulate speed for average
        total_speed_ += speed;
        speed_samples_++;
        float average_speed = (speed_samples_ > 0) ? total_speed_ / static_cast<float>(speed_samples_) : 0.0f;

        // Build and publish message
        TargetMetricsMsg msg;
        msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        msg.position = target_.position;
        msg.distance_traveled = distance_traveled_;
        msg.speed = speed;
        msg.time_elapsed = time_elapsed_;
        msg.average_speed = average_speed;

        target_.metrics_pub->publish(msg);
    }

} // namespace flychams::operator_pkg