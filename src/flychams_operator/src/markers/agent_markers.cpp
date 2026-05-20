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
    // IDLE=0 → amber, ACTIVE=1 → cyan, ERROR=2 → red
    FoxColorMsg body_color;
    if (agent_.has_status && agent_.status == 1) // ACTIVE
    {
        body_color = MarkerHelpers::makeColor(AgentParameters::kActBodyR, AgentParameters::kActBodyG, AgentParameters::kActBodyB, AgentParameters::kActBodyA);
    }
    else if (agent_.has_status && agent_.status == 2) // ERROR
    {
        body_color = MarkerHelpers::makeColor(AgentParameters::kErrBodyR, AgentParameters::kErrBodyG, AgentParameters::kErrBodyB, AgentParameters::kErrBodyA);
    }
    else // IDLE
    {
        body_color = MarkerHelpers::makeColor(AgentParameters::kIdleBodyR, AgentParameters::kIdleBodyG, AgentParameters::kIdleBodyB, AgentParameters::kIdleBodyA);
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

    // ── 1. Solid body sphere ───────────────────
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

    // ── 2. Text label ─────────────────────────────────────
    if (AgentParameters::kDisplayText)
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = pos;
        text.pose.position.z += AgentParameters::kLabelZOffset;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = AgentParameters::kFontSize;
        text.scale_invariant = false;
        text.color = MarkerHelpers::white();
        text.text = agent_id_ + "\nh=" + oss.str() + " m";
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