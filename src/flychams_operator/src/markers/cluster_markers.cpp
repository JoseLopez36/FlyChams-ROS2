#include "flychams_operator/markers/cluster_markers.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

using namespace flychams::common;

using namespace flychams::operator_pkg;

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
    scene_pub_ = node_->createScenePublisher(element_id_);

    // Subscribers
    geometry_sub_ = node_->createClusterGeometrySubscriber(cluster_id_,
        std::bind(&ClusterMarkers::geometryCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&ClusterMarkers::update, this));
}

void ClusterMarkers::onModuleShutdown()
{
    update_timer_.reset();
    scene_pub_.reset();
    geometry_sub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void ClusterMarkers::geometryCallback(const ClusterGeometryMsg::SharedPtr msg)
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
    if (!isDataValid())
    {
        return;
    }

    const std::string& frame = node_->getGlobalFrame();
    const auto& c = cluster_.center;
    const float r = cluster_.radius > 0.0f ? cluster_.radius : 1.0f;
    const auto stamp = node_->now().nanoseconds();
    const auto lifetime = rclcpp::Duration::from_seconds(2.0 / update_rate_);

    // Cluster colors: vivid green
    FoxColorMsg vol_color;
    vol_color.r = 0.18f; vol_color.g = 1.0f; vol_color.b = 0.45f; vol_color.a = 0.10f;
    FoxColorMsg ring_color;
    ring_color.r = 0.18f; ring_color.g = 1.0f; ring_color.b = 0.45f; ring_color.a = 0.85f;
    FoxColorMsg radius_color;
    radius_color.r = 0.18f; radius_color.g = 0.80f; radius_color.b = 0.30f; radius_color.a = 0.60f;
    FoxColorMsg label_color;
    label_color.r = 0.3f; label_color.g = 1.0f; label_color.b = 0.55f; label_color.a = 0.95f;

    // ── Build entity ───────────────────────────────────────────────────────
    FoxSceneEntityMsg entity;
    entity.timestamp.nanosec = static_cast<uint32_t>(stamp % 1000000000ULL);
    entity.timestamp.sec     = static_cast<int32_t>(stamp / 1000000000ULL);
    entity.frame_id = frame;
    entity.id = cluster_id_;
    entity.lifetime.sec = static_cast<int32_t>(lifetime.seconds());
    entity.lifetime.nanosec = static_cast<uint32_t>(lifetime.nanoseconds() % 1000000000LL);
    entity.frame_locked = false;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << r;
    FoxKeyValuePairMsg kv_r;
    kv_r.key = "radius_m";
    kv_r.value = oss.str();
    entity.metadata = {kv_r};

    // ── 1. Transparent bounding volume sphere ─────────────────────────────
    {
        FoxSpherePrimitiveMsg vol;
        vol.pose.position = c;
        vol.pose.orientation.w = 1.0;
        vol.size.x = r * 2.0;
        vol.size.y = r * 2.0;
        vol.size.z = r * 2.0;
        vol.color = vol_color;
        entity.spheres.push_back(vol);
    }

    // ── 2. Equatorial ring (LINE_LOOP) ────────────────────────────────────
    {
        FoxLinePrimitiveMsg ring;
        ring.type = FoxLinePrimitiveMsg::LINE_LOOP;
        ring.pose.position = c;
        ring.pose.orientation.w = 1.0;
        ring.thickness = 0.12f;
        ring.color = ring_color;
        constexpr int N = 48;
        for (int i = 0; i < N; ++i)
        {
            const double angle = 2.0 * M_PI * i / N;
            PointMsg p;
            p.x = r * std::cos(angle);
            p.y = r * std::sin(angle);
            p.z = 0.0;
            ring.points.push_back(p);
        }
        entity.lines.push_back(ring);
    }

    // ── 3. Vertical meridian ring ─────────────────────────────────────────
    {
        FoxLinePrimitiveMsg meridian;
        meridian.type = FoxLinePrimitiveMsg::LINE_LOOP;
        meridian.pose.position = c;
        meridian.pose.orientation.w = 1.0;
        meridian.thickness = 0.06f;
        meridian.color = radius_color;
        constexpr int N = 48;
        for (int i = 0; i < N; ++i)
        {
            const double angle = 2.0 * M_PI * i / N;
            PointMsg p;
            p.x = r * std::cos(angle);
            p.y = 0.0;
            p.z = r * std::sin(angle);
            meridian.points.push_back(p);
        }
        entity.lines.push_back(meridian);
    }

    // ── 4. Radius indicator line (center → equator) ───────────────────────
    {
        FoxLinePrimitiveMsg rad;
        rad.type = FoxLinePrimitiveMsg::LINE_STRIP;
        rad.pose.position = c;
        rad.pose.orientation.w = 1.0;
        rad.thickness = 0.08f;
        rad.color = radius_color;
        PointMsg p0; p0.x = 0.0; p0.y = 0.0; p0.z = 0.0;
        PointMsg p1; p1.x = r;   p1.y = 0.0; p1.z = 0.0;
        rad.points = {p0, p1};
        entity.lines.push_back(rad);
    }

    // ── 5. Text label (ID + radius) ───────────────────────────────────────
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = c;
        text.pose.position.z += r + 0.6;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = 0.55f;
        text.scale_invariant = false;
        text.color = label_color;
        text.text = cluster_id_ + "\nr=" + oss.str() + " m";
        entity.texts.push_back(text);
    }

    FoxSceneUpdateMsg update_msg;
    update_msg.entities.push_back(entity);
    scene_pub_->publish(update_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS CHECK
// ════════════════════════════════════════════════════════════════════════════

bool ClusterMarkers::isDataValid() const
{
    if (!cluster_.has_geometry)
    {
        return false;
    }
    return true;
}