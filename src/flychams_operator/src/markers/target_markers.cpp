#include "flychams_operator/markers/target_markers.hpp"

#include <sstream>
#include <iomanip>

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

    // Target colors: warm red body, soft coral glow
    FoxColorMsg body_color;
    body_color.r = 1.0f; body_color.g = 0.22f; body_color.b = 0.18f; body_color.a = 1.0f;
    FoxColorMsg glow_color;
    glow_color.r = 1.0f; glow_color.g = 0.30f; glow_color.b = 0.10f; glow_color.a = 0.16f;
    FoxColorMsg ring_color;
    ring_color.r = 1.0f; ring_color.g = 0.55f; ring_color.b = 0.0f;  ring_color.a = 0.80f;

    // ── Build entity ───────────────────────────────────────────────────────
    FoxSceneEntityMsg entity;
    entity.timestamp.nanosec = static_cast<uint32_t>(stamp % 1000000000ULL);
    entity.timestamp.sec     = static_cast<int32_t>(stamp / 1000000000ULL);
    entity.frame_id = frame;
    entity.id = target_id_;
    entity.lifetime.sec = static_cast<int32_t>(lifetime.seconds());
    entity.lifetime.nanosec = static_cast<uint32_t>(lifetime.nanoseconds() % 1000000000LL);
    entity.frame_locked = false;

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
        body.pose.position.z += 0.9;
        body.pose.orientation.w = 1.0;
        body.size.x = 0.5;
        body.size.y = 0.5;
        body.size.z = 1.8;
        body.color = body_color;
        body.top_scale = 1.0f;
        body.bottom_scale = 1.0f;
        entity.cylinders.push_back(body);
    }

    // ── 2. Transparent glow shell ──────────────────────────────────────────
    {
        FoxCylinderPrimitiveMsg glow;
        glow.pose.position = pos;
        glow.pose.position.z += 0.9;
        glow.pose.orientation.w = 1.0;
        glow.size.x = 1.4;
        glow.size.y = 1.4;
        glow.size.z = 2.4;
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
        ring.thickness = 0.06f;
        ring.color = ring_color;
        constexpr int N = 32;
        constexpr double R = 1.0;
        for (int i = 0; i < N; ++i)
        {
            const double angle = 2.0 * M_PI * i / N;
            PointMsg p;
            p.x = R * std::cos(angle);
            p.y = R * std::sin(angle);
            p.z = 0.02;
            ring.points.push_back(p);
        }
        entity.lines.push_back(ring);
    }

    // ── 4. Text label (ID) ─────────────────────────────────────────────────
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = pos;
        text.pose.position.z += 2.4;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = 0.50f;
        text.scale_invariant = false;
        FoxColorMsg white;
        white.r = 1.0f; white.g = 1.0f; white.b = 1.0f; white.a = 0.95f;
        text.color = white;
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