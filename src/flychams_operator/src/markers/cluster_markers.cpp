#include "flychams_operator/markers/cluster_markers.hpp"

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

    // Cluster colors
    const FoxColorMsg vol_color    = MarkerHelpers::makeColor(ClusterParameters::kR, ClusterParameters::kG, ClusterParameters::kB, ClusterParameters::kVolumeAlpha);
    const FoxColorMsg ring_color   = MarkerHelpers::makeColor(ClusterParameters::kR, ClusterParameters::kG, ClusterParameters::kB, ClusterParameters::kRingAlpha);
    const FoxColorMsg label_color  = MarkerHelpers::makeColor(0.3f,  ClusterParameters::kG, 0.55f,  0.95f);

    // ── Build entity ───────────────────────────────────────────────────────
    FoxSceneEntityMsg entity;
    MarkerHelpers::stampEntity(entity, stamp, lifetime);
    entity.frame_id = frame;
    entity.id = cluster_id_;

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

    // ── 2. Equatorial ring ────────────────────────────────────
    {
        FoxLinePrimitiveMsg ring;
        ring.type = FoxLinePrimitiveMsg::LINE_LOOP;
        ring.pose.position = c;
        ring.pose.orientation.w = 1.0;
        ring.thickness = ClusterParameters::kRingThickness;
        ring.color = ring_color;
        for (int i = 0; i < ClusterParameters::kRingSegments; ++i)
        {
            const double angle = 2.0 * M_PI * i / ClusterParameters::kRingSegments;
            PointMsg p;
            p.x = r * std::cos(angle);
            p.y = r * std::sin(angle);
            p.z = 0.0;
            ring.points.push_back(p);
        }
        entity.lines.push_back(ring);
    }

    // ── 3. Text label ───────────────────────────────────────
    if (ClusterParameters::kDisplayText)
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = c;
        text.pose.position.z += r + ClusterParameters::kLabelZExtraOffset;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = ClusterParameters::kFontSize;
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