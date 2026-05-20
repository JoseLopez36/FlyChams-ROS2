#include "flychams_operator/markers/agent_markers.hpp"

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
    // IDLE=0 → amber, ACTIVE=1 → cyan, ERROR=2 → red  (see AgentParams)
    FoxColorMsg body_color;
    FoxColorMsg glow_color;
    if (agent_.has_status && agent_.status == 1 /* ACTIVE */)
    {
        body_color = MarkerHelpers::makeColor(AgentParameters::kActBodyR, AgentParameters::kActBodyG, AgentParameters::kActBodyB, AgentParameters::kActBodyA);
        glow_color = MarkerHelpers::makeColor(AgentParameters::kActGlowR, AgentParameters::kActGlowG, AgentParameters::kActGlowB, AgentParameters::kActGlowA);
    }
    else if (agent_.has_status && agent_.status == 2 /* ERROR */)
    {
        body_color = MarkerHelpers::makeColor(AgentParameters::kErrBodyR, AgentParameters::kErrBodyG, AgentParameters::kErrBodyB, AgentParameters::kErrBodyA);
        glow_color = MarkerHelpers::makeColor(AgentParameters::kErrGlowR, AgentParameters::kErrGlowG, AgentParameters::kErrGlowB, AgentParameters::kErrGlowA);
    }
    else // IDLE
    {
        body_color = MarkerHelpers::makeColor(AgentParameters::kIdleBodyR, AgentParameters::kIdleBodyG, AgentParameters::kIdleBodyB, AgentParameters::kIdleBodyA);
        glow_color = MarkerHelpers::makeColor(AgentParameters::kIdleGlowR, AgentParameters::kIdleGlowG, AgentParameters::kIdleGlowB, AgentParameters::kIdleGlowA);
    }

    // ── Build entity ───────────────────────────────────────────────────────
    FoxSceneEntityMsg entity;
    MarkerHelpers::stampEntity(entity, stamp, lifetime);
    entity.frame_id = frame;
    entity.id = agent_id_;

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

    // ── 1. Solid body sphere (flattened UAV silhouette) ───────────────────
    {
        FoxSpherePrimitiveMsg sphere;
        sphere.pose.position = pos;
        sphere.pose.orientation.w = 1.0;
        sphere.size.x = AgentParameters::kBodyDiamXY;
        sphere.size.y = AgentParameters::kBodyDiamXY;
        sphere.size.z = AgentParameters::kBodyDiamZ;
        sphere.color = body_color;
        entity.spheres.push_back(sphere);
    }

    // ── 2. Semi-transparent glow shell ────────────────────────────────────
    {
        FoxSpherePrimitiveMsg glow;
        glow.pose.position = pos;
        glow.pose.orientation.w = 1.0;
        glow.size.x = AgentParameters::kGlowDiamXY;
        glow.size.y = AgentParameters::kGlowDiamXY;
        glow.size.z = AgentParameters::kGlowDiamZ;
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
        arrow.shaft_length   = AgentParameters::kArrowShaftLen;
        arrow.shaft_diameter = AgentParameters::kArrowShaftDiam;
        arrow.head_length    = AgentParameters::kArrowHeadLen;
        arrow.head_diameter  = AgentParameters::kArrowHeadDiam;
        arrow.color = body_color;
        entity.arrows.push_back(arrow);
    }

    // ── 4. Text label (ID + altitude) ─────────────────────────────────────
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = pos;
        text.pose.position.z += AgentParameters::kLabelZOffset;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = AgentParameters::kFontSize;
        text.scale_invariant = false;
        text.color = MarkerHelpers::white();
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