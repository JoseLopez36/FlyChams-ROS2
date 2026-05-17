#include "flychams_operator/metrics/cluster_metrics.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void ClusterMetrics::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Initialize data
    cluster_ = ClusterData();
    distance_traveled_ = 0.0f;
    total_speed_ = 0.0f;
    speed_samples_ = 0;
    time_elapsed_ = 0.0f;
    last_update_time_ = node_->now();

    // Publishers
    cluster_.metrics_pub = node_->createClusterMetricsPublisher(cluster_id_);

    // Subscribers
    cluster_.geometry_sub = node_->createClusterGeometrySubscriber(cluster_id_,
        std::bind(&ClusterMetrics::clusterGeometryCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&ClusterMetrics::update, this));
}

void ClusterMetrics::onModuleShutdown()
{
    cluster_.metrics_pub.reset();
    cluster_.geometry_sub.reset();
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void ClusterMetrics::clusterGeometryCallback(const ClusterGeometryMsg::SharedPtr msg)
{
    if (cluster_.has_geometry)
    {
        float dx = msg->center.x - cluster_.center.x;
        float dy = msg->center.y - cluster_.center.y;
        float dz = msg->center.z - cluster_.center.z;
        distance_traveled_ += std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    else
    {
        last_center_ = msg->center;
    }
    cluster_.center = msg->center;
    cluster_.radius = msg->radius;
    cluster_.has_geometry = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void ClusterMetrics::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Cluster metrics: Skipping update due to invalid status");
        return;
    }
    // Compute dt
    auto now = node_->now();
    float dt = static_cast<float>((now - last_update_time_).seconds());
    last_update_time_ = now;
    time_elapsed_ += dt;

    // Compute instantaneous centroid speed
    float dx = cluster_.center.x - last_center_.x;
    float dy = cluster_.center.y - last_center_.y;
    float dz = cluster_.center.z - last_center_.z;
    float speed = (dt > 0.0f) ? std::sqrt(dx * dx + dy * dy + dz * dz) / dt : 0.0f;
    last_center_ = cluster_.center;

    // Accumulate speed for average
    total_speed_ += speed;
    speed_samples_++;
    float average_speed = (speed_samples_ > 0) ? total_speed_ / static_cast<float>(speed_samples_) : 0.0f;

    // Build and publish message
    ClusterMetricsMsg msg;
    msg.header = node_->createHeader(node_->getGlobalFrame());
    msg.center = cluster_.center;
    msg.radius = cluster_.radius;
    msg.distance_traveled = distance_traveled_;
    msg.speed = speed;
    msg.time_elapsed = time_elapsed_;
    msg.average_speed = average_speed;

    cluster_.metrics_pub->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool ClusterMetrics::checkStatus()
{
    // Check 1: Mission must be active
    if (!node_->isMissionActive())
    {
        RCLCPP_WARN(node_->get_logger(), "Target clustering: Mission is not active");
        return false;
    }

    // Check 2: Cluster must have a valid geometry
    if (!cluster_.has_geometry)
    {
        return false;
    }

    // All checks passed
    return true;
}