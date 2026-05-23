#include "flychams_operator/markers/target_markers.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void TargetMarkers::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Initialize data
    target_ = TargetData();

    // Subscribers
    position_sub_ = node_->createTargetPositionSubscriber(target_id_,
        std::bind(&TargetMarkers::positionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());
}

void TargetMarkers::onModuleShutdown()
{
    position_sub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void TargetMarkers::positionCallback(const PointStampedMsg::SharedPtr msg)
{
    target_.position = msg->point;
    target_.has_position = true;
}

// ════════════════════════════════════════════════════════════════════════════
// ENTITY COLLECTION
// ════════════════════════════════════════════════════════════════════════════

void TargetMarkers::getEntities(FoxSceneUpdateMsg& out) const
{
    if (!target_.has_position)
    {
        return;
    }

    const std::string& frame = node_->getGlobalFrame();
    const auto& pos = target_.position;
    const auto stamp = node_->now().nanoseconds();
    const auto lifetime = rclcpp::Duration::from_seconds(2.0 / update_rate_);

    // ── Build entity ──────────────────────────────────────────────────────
    FoxSceneEntityMsg entity;
    MarkerHelpers::stampEntity(entity, stamp, lifetime);
    entity.frame_id = frame;
    entity.id = target_id_;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << pos.z;
    FoxKeyValuePairMsg kv_alt;
    kv_alt.key = "alt_m";
    kv_alt.value = oss.str();
    entity.metadata = {kv_alt};

    // ── 1. Cylinder body ──────────────────────────────────────────────────
    {
        FoxCylinderPrimitiveMsg body;
        body.pose.position = pos;
        body.pose.position.z += TargetParameters::kBodyZOffset;
        body.pose.orientation.w = 1.0;
        body.size.x = TargetParameters::kBodyDiamXY;
        body.size.y = TargetParameters::kBodyDiamXY;
        body.size.z = TargetParameters::kBodyHeight;
        body.color = TargetParameters::kBody;
        body.top_scale = 1.0f;
        body.bottom_scale = 1.0f;
        entity.cylinders.push_back(body);
    }

    // ── 2. Text label ─────────────────────────────────────────────────────
    if (TargetParameters::kDisplayText)
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = pos;
        text.pose.position.z += TargetParameters::kLabelZOffset;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = TargetParameters::kFontSize;
        text.scale_invariant = false;
        text.color = Colors::kWhite;
        text.text = target_id_;
        entity.texts.push_back(text);
    }

    out.entities.push_back(entity);
}