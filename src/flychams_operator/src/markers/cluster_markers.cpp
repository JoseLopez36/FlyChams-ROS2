#include "flychams_operator/markers/cluster_markers.hpp"

using namespace flychams::core;

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // INIT / SHUTDOWN
    // ════════════════════════════════════════════════════════════════════════════

    void ClusterMarkers::onModuleInit()
    {
        // Get parameters
        update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

        // Initialize data
        cluster_ = ClusterData();

        // Publishers
        cluster_.markers_pub = node_->createClusterMarkersPublisher(cluster_id_);

        // Subscribers
        cluster_.geometry_sub = node_->createClusterGeometrySubscriber(cluster_id_,
            std::bind(&ClusterMarkers::clusterGeometryCallback, this, std::placeholders::_1),
            node_->getSubscriptionOptions());

        // Update timer
        update_timer_ = node_->createTimer(update_rate_, std::bind(&ClusterMarkers::update, this));
    }

    void ClusterMarkers::onModuleShutdown()
    {
        cluster_.markers_pub.reset();
        cluster_.geometry_sub.reset();
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS
    // ════════════════════════════════════════════════════════════════════════════

    void ClusterMarkers::clusterGeometryCallback(const ClusterGeometryMsg::SharedPtr msg)
    {
        cluster_.center = msg->center;
        cluster_.radius = msg->radius;
        cluster_.has_geometry = true;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE
    // ════════════════════════════════════════════════════════════════════════════

    void ClusterMarkers::update()
    {
        if (!cluster_.has_geometry)
        {
            return;
        }

        const std::string& frame = node_->getGlobalFrame();
        auto stamp = node_->now();
        const float r = cluster_.radius > 0.0f ? cluster_.radius : 1.0f;

        MarkerArrayMsg array;

        // ── Sphere marker representing the cluster bounding volume ─────────────
        MarkerMsg sphere;
        sphere.header.frame_id = frame;
        sphere.header.stamp = stamp;
        sphere.ns = cluster_id_;
        sphere.id = 0;
        sphere.type = MarkerMsg::SPHERE;
        sphere.action = MarkerMsg::ADD;
        sphere.pose.position = cluster_.center;
        sphere.pose.orientation.w = 1.0;
        sphere.scale.x = r * 2.0f;
        sphere.scale.y = r * 2.0f;
        sphere.scale.z = r * 2.0f;
        sphere.color.r = 0.2f; sphere.color.g = 1.0f; sphere.color.b = 0.4f; sphere.color.a = 0.25f;
        array.markers.push_back(sphere);

        // ── Wireframe circle (LINE_STRIP) at cluster equator ──────────────────
        MarkerMsg ring;
        ring.header.frame_id = frame;
        ring.header.stamp = stamp;
        ring.ns = cluster_id_ + "_ring";
        ring.id = 1;
        ring.type = MarkerMsg::LINE_STRIP;
        ring.action = MarkerMsg::ADD;
        ring.pose.orientation.w = 1.0;
        ring.scale.x = 0.1;
        ring.color.r = 0.2f; ring.color.g = 1.0f; ring.color.b = 0.4f; ring.color.a = 0.8f;
        constexpr int N_RING = 32;
        for (int i = 0; i <= N_RING; ++i)
        {
            float angle = static_cast<float>(i) / static_cast<float>(N_RING) * 2.0f * M_PIf32;
            PointMsg p;
            p.x = cluster_.center.x + r * std::cos(angle);
            p.y = cluster_.center.y + r * std::sin(angle);
            p.z = cluster_.center.z;
            ring.points.push_back(p);
        }
        array.markers.push_back(ring);

        // ── Text label ─────────────────────────────────────────────────────────
        MarkerMsg label;
        label.header.frame_id = frame;
        label.header.stamp = stamp;
        label.ns = cluster_id_ + "_label";
        label.id = 2;
        label.type = MarkerMsg::TEXT_VIEW_FACING;
        label.action = MarkerMsg::ADD;
        label.pose.position = cluster_.center;
        label.pose.position.z += r + 0.5f;
        label.pose.orientation.w = 1.0;
        label.scale.z = 0.7;
        label.color.r = 0.2f; label.color.g = 1.0f; label.color.b = 0.4f; label.color.a = 1.0f;
        label.text = cluster_id_;
        array.markers.push_back(label);

        cluster_.markers_pub->publish(array);
    }

} // namespace flychams::operator_pkg