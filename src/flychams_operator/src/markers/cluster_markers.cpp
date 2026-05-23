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
    agent_clusters_.clear();

    // Publisher
    scene_pub_ = node_->createScenePublisher(element_id_);

    // Subscribe to agent clusters
    for (const auto& [agent_id, agent_idx] : agent_index_map_)
    {
        AgentClusterData data;
        data.agent_idx = agent_idx;
        agent_clusters_[agent_id] = data;

        clusters_subs_.push_back(
            node_->createAgentClustersSubscriber(agent_id,
                [this, agent_id](const AgentClustersMsg::SharedPtr msg)
                {
                    clustersCallback(agent_id, msg);
                },
                node_->getSubscriptionOptions()));
    }

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&ClusterMarkers::update, this));
}

void ClusterMarkers::onModuleShutdown()
{
    update_timer_.reset();
    scene_pub_.reset();
    clusters_subs_.clear();
    agent_clusters_.clear();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void ClusterMarkers::clustersCallback(const ID& agent_id, const AgentClustersMsg::SharedPtr msg)
{
    if (msg->centers.empty())
        return;

    auto& data = agent_clusters_[agent_id];
    data.clusters.clear();
    const size_t n = msg->centers.size();
    for (size_t i = 1; i < n; ++i)  // skip index 0 (global cluster)
    {
        ClusterData cluster;
        cluster.center  = msg->centers[i];
        cluster.radius  = msg->radii[i];
        cluster.unit_id = msg->unit_ids[i];
        data.clusters.push_back(cluster);
    }
    data.has_data = true;
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

    FoxSceneUpdateMsg update_msg;

    const std::string& frame = node_->getGlobalFrame();
    const auto stamp_ns = node_->now().nanoseconds();
    const auto lifetime = rclcpp::Duration::from_seconds(2.0 / update_rate_);

    // Create each agent's clusters
    for (const auto& [agent_id, data] : agent_clusters_)
    {
        if (!data.has_data)
            continue;

        for (size_t i = 0; i < data.clusters.size(); ++i)
        {
            buildClusterEntity(agent_id, i, data, frame, stamp_ns, lifetime, update_msg);
        }
    }

    scene_pub_->publish(update_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// ENTITY BUILDER
// ════════════════════════════════════════════════════════════════════════════

void ClusterMarkers::buildClusterEntity(const ID& agent_id, size_t entry_idx, const AgentClusterData& data,
                                        const std::string& frame, int64_t stamp_ns,
                                        const rclcpp::Duration& lifetime,
                                        FoxSceneUpdateMsg& out) const
{
    const auto& cluster = data.clusters[entry_idx];
    const auto& c = cluster.center;
    const float r = cluster.radius > 0.0f ? cluster.radius : 1.0f;

    // Pick agent color from palette
    const Color& base_color = AgentColors::get(data.color_idx);
    const FoxColorMsg vol_color   = base_color; vol_color.a = ClusterParameters::kVolumeAlpha;
    const FoxColorMsg ring_color  = base_color; ring_color.a = ClusterParameters::kRingAlpha;
    const FoxColorMsg label_color = base_color; label_color.a = 0.75f;

    // Unique entity ID per agent within this cluster scene
    FoxSceneEntityMsg entity;
    MarkerHelpers::stampEntity(entity, stamp_ns, lifetime);
    entity.frame_id = frame;
    entity.id = cluster_id_ + "/" + agent_id + "/" + cluster.unit_id;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << r;
    FoxKeyValuePairMsg kv;
    kv.key   = "agent";
    kv.value = agent_id;
    entity.metadata = {kv};

    // ── 1. Transparent bounding volume sphere ─────────────────────────────
    {
        FoxSpherePrimitiveMsg vol;
        vol.pose.position    = c;
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
        FoxTextPrimitiveMsg text;
        text.pose.position    = c;
        text.pose.position.z += r + ClusterParameters::kLabelZExtraOffset;
        text.pose.orientation.w = 1.0;
        text.billboard       = true;
        text.font_size       = ClusterParameters::kFontSize;
        text.scale_invariant = false;
        text.color           = label_color;
        text.text            = agent_id + " - " + cluster.unit_id + "\nr=" + oss.str() + " m";
        entity.texts.push_back(text);
    }

    out.entities.push_back(entity);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS CHECK
// ════════════════════════════════════════════════════════════════════════════

bool ClusterMarkers::isDataValid() const
{
    for (const auto& [id, data] : agent_clusters_)
    {
        if (data.has_data)
            return true;
    }
    return false;
}