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

    // Publishers
    scene_pub_ = node_->createScenePublisher(element_id_);

    // Subscribers
    position_sub_ = node_->createTargetPositionSubscriber(target_id_,
        std::bind(&TargetMarkers::positionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&TargetMarkers::update, this));
}

void TargetMarkers::onModuleShutdown()
{
    update_timer_.reset();
    scene_pub_.reset();
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
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void TargetMarkers::update()
{
    if (!isDataValid())
    {
        return;
    }

    const std::string& frame = node_->getGlobalFrame();
    const auto& pos = target_.position;
    const auto stamp = node_->now().nanoseconds();
    const auto lifetime = rclcpp::Duration::from_seconds(2.0 / update_rate_);

    // Target colors (see TargetParams)
    const FoxColorMsg body_color = MarkerHelpers::makeColor(TargetParameters::kBodyR, TargetParameters::kBodyG, TargetParameters::kBodyB, TargetParameters::kBodyA);
    const FoxColorMsg glow_color = MarkerHelpers::makeColor(TargetParameters::kGlowR, TargetParameters::kGlowG, TargetParameters::kGlowB, TargetParameters::kGlowA);
    const FoxColorMsg ring_color = MarkerHelpers::makeColor(TargetParameters::kRingR, TargetParameters::kRingG, TargetParameters::kRingB, TargetParameters::kRingA);

    // ── Build entity ───────────────────────────────────────────────────────
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

    // ── 1. Human-shaped cylinder body ─────────────────────────────────────
    {
        FoxCylinderPrimitiveMsg body;
        body.pose.position = pos;
        body.pose.position.z += TargetParameters::kBodyZOffset;
        body.pose.orientation.w = 1.0;
        body.size.x = TargetParameters::kBodyDiamXY;
        body.size.y = TargetParameters::kBodyDiamXY;
        body.size.z = TargetParameters::kBodyHeight;
        body.color = body_color;
        body.top_scale = 1.0f;
        body.bottom_scale = 1.0f;
        entity.cylinders.push_back(body);
    }

    // ── 2. Transparent glow shell ──────────────────────────────────────────
    {
        FoxCylinderPrimitiveMsg glow;
        glow.pose.position = pos;
        glow.pose.position.z += TargetParameters::kGlowZOffset;
        glow.pose.orientation.w = 1.0;
        glow.size.x = TargetParameters::kGlowDiamXY;
        glow.size.y = TargetParameters::kGlowDiamXY;
        glow.size.z = TargetParameters::kGlowHeight;
        glow.color = glow_color;
        glow.top_scale = 1.0f;
        glow.bottom_scale = 1.0f;
        entity.cylinders.push_back(glow);
    }

    // ── 3. Ground-plane detection ring (LinePrimitive loop) ───────────────
    {
        FoxLinePrimitiveMsg ring;
        ring.type = FoxLinePrimitiveMsg::LINE_LOOP;
        ring.pose.position = pos;
        ring.pose.orientation.w = 1.0;
        ring.thickness = TargetParameters::kRingThickness;
        ring.color = ring_color;
        for (int i = 0; i < TargetParameters::kRingSegments; ++i)
        {
            const double angle = 2.0 * M_PI * i / TargetParameters::kRingSegments;
            PointMsg p;
            p.x = TargetParameters::kRingRadius * std::cos(angle);
            p.y = TargetParameters::kRingRadius * std::sin(angle);
            p.z = 0.02;
            ring.points.push_back(p);
        }
        entity.lines.push_back(ring);
    }

    // ── 4. Text label (ID) ─────────────────────────────────────────────────
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = pos;
        text.pose.position.z += TargetParameters::kLabelZOffset;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = TargetParameters::kFontSize;
        text.scale_invariant = false;
        text.color = MarkerHelpers::white();
        text.text = target_id_;
        entity.texts.push_back(text);
    }

    FoxSceneUpdateMsg update_msg;
    update_msg.entities.push_back(entity);
    scene_pub_->publish(update_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS CHECK
// ════════════════════════════════════════════════════════════════════════════

bool TargetMarkers::isDataValid() const
{
    if (!target_.has_position)
    {
        return false;
    }
    return true;
}