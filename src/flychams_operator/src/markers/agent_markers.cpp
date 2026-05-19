#include "flychams_operator/markers/agent_markers.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void AgentMarkers::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Initialize data
    agent_ = AgentData();

    // Publishers
    scene_pub_ = node_->createScenePublisher(element_id_);

    // Subscribers
    position_sub_ = node_->createAgentLocalPositionSubscriber(agent_id_,
        std::bind(&AgentMarkers::positionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    status_sub_ = node_->createAgentStatusSubscriber(agent_id_,
        std::bind(&AgentMarkers::statusCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentMarkers::update, this));
}

void AgentMarkers::onModuleShutdown()
{
    update_timer_.reset();
    scene_pub_.reset();
    position_sub_.reset();
    status_sub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void AgentMarkers::positionCallback(const PointStampedMsg::SharedPtr msg)
{
    agent_.position = msg->point;
    agent_.has_position = true;
}

void AgentMarkers::statusCallback(const AgentStatusMsg::SharedPtr msg)
{
    agent_.status = msg->status;
    agent_.has_status = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void AgentMarkers::update()
{
    if (!isDataValid())
    {
        return;
    }

    const std::string& frame = node_->getGlobalFrame();
    const auto& pos = agent_.position;
    const auto stamp = node_->now().nanoseconds();
    const auto lifetime = rclcpp::Duration::from_seconds(2.0 / update_rate_);

    // ── Determine status color ─────────────────────────────────────────────
    // IDLE=0 → yellow-amber, ACTIVE=1 → bright cyan, ERROR=2 → red
    FoxColorMsg body_color;
    FoxColorMsg glow_color;
    if (agent_.has_status && agent_.status == 1 /* ACTIVE */)
    {
        body_color.r = 0.0f; body_color.g = 0.85f; body_color.b = 1.0f; body_color.a = 1.0f;
        glow_color.r = 0.0f; glow_color.g = 0.65f; glow_color.b = 1.0f; glow_color.a = 0.18f;
    }
    else if (agent_.has_status && agent_.status == 2 /* ERROR */)
    {
        body_color.r = 1.0f; body_color.g = 0.15f; body_color.b = 0.1f; body_color.a = 1.0f;
        glow_color.r = 1.0f; glow_color.g = 0.1f;  glow_color.b = 0.0f; glow_color.a = 0.22f;
    }
    else // IDLE
    {
        body_color.r = 1.0f; body_color.g = 0.78f; body_color.b = 0.0f; body_color.a = 1.0f;
        glow_color.r = 1.0f; glow_color.g = 0.65f; glow_color.b = 0.0f; glow_color.a = 0.14f;
    }

    // ── Build entity ───────────────────────────────────────────────────────
    FoxSceneEntityMsg entity;
    entity.timestamp.nanosec = static_cast<uint32_t>(stamp % 1000000000ULL);
    entity.timestamp.sec     = static_cast<int32_t>(stamp / 1000000000ULL);
    entity.frame_id = frame;
    entity.id = agent_id_;
    entity.lifetime.sec = static_cast<int32_t>(lifetime.seconds());
    entity.lifetime.nanosec = static_cast<uint32_t>(lifetime.nanoseconds() % 1000000000LL);
    entity.frame_locked = false;

    // Metadata
    FoxKeyValuePairMsg kv_status;
    kv_status.key = "status";
    kv_status.value = std::to_string(agent_.status);
    FoxKeyValuePairMsg kv_alt;
    kv_alt.key = "alt_m";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << pos.z;
    kv_alt.value = oss.str();
    entity.metadata = {kv_status, kv_alt};

    // ── 1. Solid body sphere (1 m diameter, flattened UAV silhouette) ──────
    {
        FoxSpherePrimitiveMsg sphere;
        sphere.pose.position = pos;
        sphere.pose.orientation.w = 1.0;
        sphere.size.x = 1.0;
        sphere.size.y = 1.0;
        sphere.size.z = 0.35;
        sphere.color = body_color;
        entity.spheres.push_back(sphere);
    }

    // ── 2. Semi-transparent glow shell ────────────────────────────────────
    {
        FoxSpherePrimitiveMsg glow;
        glow.pose.position = pos;
        glow.pose.orientation.w = 1.0;
        glow.size.x = 2.0;
        glow.size.y = 2.0;
        glow.size.z = 0.9;
        glow.color = glow_color;
        entity.spheres.push_back(glow);
    }

    // ── 3. Upward orientation arrow ────────────────────────────────────────
    {
        FoxArrowPrimitiveMsg arrow;
        arrow.pose.position = pos;
        // Arrow points along local +Z; rotate from +X to +Z: 90° around -Y
        arrow.pose.orientation.x =  0.0;
        arrow.pose.orientation.y = -0.7071068f;
        arrow.pose.orientation.z =  0.0;
        arrow.pose.orientation.w =  0.7071068f;
        arrow.shaft_length = 1.5;
        arrow.shaft_diameter = 0.10;
        arrow.head_length = 0.35;
        arrow.head_diameter = 0.28;
        arrow.color = body_color;
        entity.arrows.push_back(arrow);
    }

    // ── 4. Text label (ID + altitude) ─────────────────────────────────────
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = pos;
        text.pose.position.z += 1.6;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = 0.55f;
        text.scale_invariant = false;
        FoxColorMsg white;
        white.r = 1.0f; white.g = 1.0f; white.b = 1.0f; white.a = 0.95f;
        text.color = white;
        text.text = agent_id_ + "\n" + oss.str() + " m";
        entity.texts.push_back(text);
    }

    // ── Publish SceneUpdate ────────────────────────────────────────────────
    FoxSceneUpdateMsg update_msg;
    update_msg.entities.push_back(entity);
    scene_pub_->publish(update_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS CHECK
// ════════════════════════════════════════════════════════════════════════════

bool AgentMarkers::isDataValid() const
{
    if (!agent_.has_position)
    {
        return false;
    }
    return true;
}