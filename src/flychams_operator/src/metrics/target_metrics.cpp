#include "flychams_operator/metrics/target_metrics.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void TargetMetrics::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Initialize data
    target_ = TargetData();
    distance_traveled_ = 0.0f;
    total_speed_ = 0.0f;
    speed_samples_ = 0;
    time_elapsed_ = 0.0f;
    last_update_time_ = node_->now();

    // Publishers
    target_.metrics_pub = node_->createTargetMetricsPublisher(target_id_);

    // Subscribers
    target_.position_sub = node_->createTargetPositionSubscriber(target_id_,
        std::bind(&TargetMetrics::positionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&TargetMetrics::update, this));
}

void TargetMetrics::onModuleShutdown()
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
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Target metrics: Skipping update due to invalid status");
        return;
    }

    // Compute dt
    auto now = node_->now();
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
    msg.header = node_->createHeader(node_->getGlobalFrame());
    msg.position = target_.position;
    msg.distance_traveled = distance_traveled_;
    msg.speed = speed;
    msg.time_elapsed = time_elapsed_;
    msg.average_speed = average_speed;

    target_.metrics_pub->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool TargetMetrics::checkStatus()
{
    // Check 1: Mission must be active
    if (!node_->isMissionActive())
    {
        RCLCPP_INFO(node_->get_logger(), "Target clustering: Mission is not active");
        return false;
    }

    // Check 2: Target must have a valid position
    if (!target_.has_position)
    {
        RCLCPP_INFO(node_->get_logger(), "Target metrics: Target %s has no position", target_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}