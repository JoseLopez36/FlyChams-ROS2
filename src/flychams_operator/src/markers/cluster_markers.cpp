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

    // Resolve agent colour index from mission settings
    agent_idx_ = node_->getSettings()->getAgent(agent_id_)->idx;

    // Subscribe to this agent's clusters topic
    clusters_sub_ = node_->createAgentClustersSubscriber(agent_id_,
        std::bind(&ClusterMarkers::clustersCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());
}

void ClusterMarkers::onModuleShutdown()
{
    clusters_sub_.reset();
    clusters_.clear();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void ClusterMarkers::clustersCallback(const AgentClustersMsg::SharedPtr msg)
{
    if (msg->centers.empty())
        return;

    clusters_.clear();
    const size_t n = msg->centers.size();
    for (size_t i = 0; i < n; ++i)
    {
        ClusterData cluster;
        cluster.center  = msg->centers[i];
        cluster.radius  = msg->radii[i];
        cluster.unit_id = msg->unit_ids[i];
        clusters_.push_back(cluster);
    }
    has_data_ = true;
}

// ════════════════════════════════════════════════════════════════════════════
// ENTITY COLLECTION
// ════════════════════════════════════════════════════════════════════════════

void ClusterMarkers::getEntities(FoxSceneUpdateMsg& out) const
{
    if (!has_data_)
        return;

    const std::string& frame = node_->getGlobalFrame();
    const auto stamp_ns = node_->now().nanoseconds();
    const auto lifetime = rclcpp::Duration::from_seconds(2.0 / update_rate_);

    // Pick agent color from palette
    const Color& base_color = AgentColors::get(agent_idx_);
    Color vol_color   = base_color; vol_color.a   = ClusterParameters::kVolumeAlpha;
    Color ring_color  = base_color; ring_color.a  = ClusterParameters::kRingAlpha;
    Color label_color = base_color; label_color.a = 0.75f;

    // ── Build entity for each cluster ─────────────────────────────────────
    for (size_t i = 0; i < clusters_.size(); ++i)
    {
        FoxSceneEntityMsg entity;

        const auto& cluster = clusters_[i];
        const auto& c = cluster.center;
        const float r = cluster.radius > 0.0f ? cluster.radius : 1.0f;

        MarkerHelpers::stampEntity(entity, stamp_ns, lifetime);
        entity.frame_id = frame;
        entity.id = agent_id_ + "/" + cluster.unit_id;

        FoxKeyValuePairMsg kv;
        kv.key   = "agent";
        kv.value = agent_id_;
        entity.metadata = {kv};

        // ── 1. Transparent bounding volume sphere ─────────────────────────────
        {
            FoxSpherePrimitiveMsg vol;
            vol.pose.position      = c;
            vol.pose.orientation.w = 1.0;
            vol.size.x = r * 2.0;
            vol.size.y = r * 2.0;
            vol.size.z = r * 2.0;
            vol.color  = vol_color;
            entity.spheres.push_back(vol);
        }

        // ── 2. Equatorial ring ────────────────────────────────────────────────
        {
            FoxLinePrimitiveMsg ring;
            ring.type = FoxLinePrimitiveMsg::LINE_LOOP;
            ring.pose.position     = c;
            ring.pose.orientation.w = 1.0;
            ring.thickness = ClusterParameters::kRingThickness;
            ring.color     = ring_color;
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

        // ── 3. Text label ─────────────────────────────────────────────────────
        if (ClusterParameters::kDisplayText)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << r;
            FoxTextPrimitiveMsg text;
            text.pose.position    = c;
            text.pose.position.z += r + ClusterParameters::kLabelZExtraOffset;
            text.pose.orientation.w = 1.0;
            text.billboard       = true;
            text.font_size       = ClusterParameters::kFontSize;
            text.scale_invariant = false;
            text.color           = label_color;
            text.text            = agent_id_ + " - " + cluster.unit_id + "\nr=" + oss.str() + " m";
            entity.texts.push_back(text);
        }

        out.entities.push_back(entity);
    }
}