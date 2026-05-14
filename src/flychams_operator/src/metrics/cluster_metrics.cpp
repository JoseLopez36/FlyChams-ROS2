#include "flychams_operator/metrics/cluster_metrics.hpp"

#include "flychams_common/utils/ros_utils.hpp"

using namespace flychams::core;

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // INIT / SHUTDOWN
    // ════════════════════════════════════════════════════════════════════════════

    void ClusterMetrics::onInit()
    {
        // Get parameters
        update_rate_ = RosUtils::getParameterOr<float>(node_, "update_rate", 10.0f);

        // Initialize data
        cluster_ = ClusterData();
        distance_traveled_ = 0.0f;
        total_speed_ = 0.0f;
        speed_samples_ = 0;
        time_elapsed_ = 0.0f;
        last_update_time_ = RosUtils::now(node_);

        // Publishers
        cluster_.metrics_pub = topic_tools_->createClusterMetricsPublisher(cluster_id_);

        // Subscribers
        cluster_.geometry_sub = topic_tools_->createClusterGeometrySubscriber(cluster_id_,
            std::bind(&ClusterMetrics::clusterGeometryCallback, this, std::placeholders::_1),
            sub_options_with_module_cb_group_);

        // Update timer
        update_timer_ = rclcpp::create_timer(node_,
            node_->get_clock(),
            std::chrono::duration<float>(1.0f / update_rate_),
            std::bind(&ClusterMetrics::update, this),
            module_cb_group_);
    }

    void ClusterMetrics::onShutdown()
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
        if (!cluster_.has_geometry)
        {
            return;
        }

        // Compute dt
        auto now = RosUtils::now(node_);
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
        msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        msg.center = cluster_.center;
        msg.radius = cluster_.radius;
        msg.distance_traveled = distance_traveled_;
        msg.speed = speed;
        msg.time_elapsed = time_elapsed_;
        msg.average_speed = average_speed;

        cluster_.metrics_pub->publish(msg);
    }

} // namespace flychams::operator_pkg