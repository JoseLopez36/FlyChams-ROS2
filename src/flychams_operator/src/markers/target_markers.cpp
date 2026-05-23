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

    const Color body_color  = TargetParameters::kBody;
    Color glow_color        = body_color; glow_color.a  = TargetParameters::kGlowAlpha;
    Color ring_color        = body_color; ring_color.a  = TargetParameters::kRingAlpha;
    Color ground_color      = body_color; ground_color.a = TargetParameters::kGroundAlpha;

    // ── 1. Solid body cylinder ────────────────────────────────────────────
    {
        FoxCylinderPrimitiveMsg body;
        body.pose.position    = pos;
        body.pose.position.z += TargetParameters::kBodyZOffset;
        body.pose.orientation.w = 1.0;
        body.size.x  = TargetParameters::kBodyDiamXY;
        body.size.y  = TargetParameters::kBodyDiamXY;
        body.size.z  = TargetParameters::kBodyHeight;
        body.color   = body_color;
        body.top_scale    = 1.0f;
        body.bottom_scale = 1.0f;
        entity.cylinders.push_back(body);
    }

    // ── 2. Outer glow sphere ──────────────────────────────────────────────
    {
        FoxSpherePrimitiveMsg glow;
        glow.pose.position    = pos;
        glow.pose.position.z += TargetParameters::kBodyZOffset;
        glow.pose.orientation.w = 1.0;
        glow.size.x = TargetParameters::kGlowDiam;
        glow.size.y = TargetParameters::kGlowDiam;
        glow.size.z = TargetParameters::kGlowDiam;
        glow.color  = glow_color;
        entity.spheres.push_back(glow);
    }

    // ── 3. Equatorial ring at body mid-height ─────────────────────────────
    {
        FoxLinePrimitiveMsg ring;
        ring.type = FoxLinePrimitiveMsg::LINE_LOOP;
        ring.pose.position    = pos;
        ring.pose.position.z += TargetParameters::kBodyZOffset;
        ring.pose.orientation.w = 1.0;
        ring.thickness = TargetParameters::kRingThickness;
        ring.color     = ring_color;
        const double r = TargetParameters::kBodyDiamXY * 0.5;
        for (int i = 0; i < TargetParameters::kRingSegments; ++i)
        {
            const double angle = 2.0 * M_PI * i / TargetParameters::kRingSegments;
            PointMsg p;
            p.x = r * std::cos(angle);
            p.y = r * std::sin(angle);
            p.z = 0.0;
            ring.points.push_back(p);
        }
        entity.lines.push_back(ring);
    }

    // ── 4. Ground disc (flat cylinder at z = 0) ───────────────────────────
    {
        FoxCylinderPrimitiveMsg disc;
        disc.pose.position    = pos;
        disc.pose.position.z  = TargetParameters::kGroundHeight * 0.5;
        disc.pose.orientation.w = 1.0;
        disc.size.x  = TargetParameters::kGroundDiam;
        disc.size.y  = TargetParameters::kGroundDiam;
        disc.size.z  = TargetParameters::kGroundHeight;
        disc.color   = ground_color;
        disc.top_scale    = 1.0f;
        disc.bottom_scale = 1.0f;
        entity.cylinders.push_back(disc);
    }

    // ── 5. Text label ─────────────────────────────────────────────────────
    if (TargetParameters::kDisplayText)
    {
        FoxTextPrimitiveMsg text;
        text.pose.position    = pos;
        text.pose.position.z += TargetParameters::kLabelZOffset;
        text.pose.orientation.w = 1.0;
        text.billboard       = true;
        text.font_size       = TargetParameters::kFontSize;
        text.scale_invariant = false;
        text.color           = TargetParameters::kLabel;
        text.text            = target_id_;
        entity.texts.push_back(text);
    }

    out.entities.push_back(entity);
}