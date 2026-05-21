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
    position_sub_ = node_->createAgentGlobalPositionSubscriber(agent_id_,
        std::bind(&AgentMarkers::positionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    status_sub_ = node_->createAgentStatusSubscriber(agent_id_,
        std::bind(&AgentMarkers::statusCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    setpoint_sub_ = node_->createAgentPositionSetpointSubscriber(agent_id_,
        std::bind(&AgentMarkers::setpointCallback, this, std::placeholders::_1),
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
    setpoint_sub_.reset();
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

void AgentMarkers::setpointCallback(const PointStampedMsg::SharedPtr msg)
{
    agent_.setpoint = msg->point;
    agent_.has_setpoint = true;
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

    FoxSceneUpdateMsg update_msg;

    const std::string frame = node_->getAgentBodyFrame(agent_id_);
    const std::string global_frame = node_->getGlobalFrame();
    const auto& pos = agent_.position;
    const auto stamp = node_->now().nanoseconds();
    const auto lifetime = rclcpp::Duration::from_seconds(2.0 / update_rate_);

    // ── Status colors: IDLE, ACTIVE, ERROR ─────────────────
    FoxColorMsg body_color;
    FoxColorMsg rotor_color;
    if (agent_.has_status && agent_.status == 1) // ACTIVE
    {
        body_color  = MarkerHelpers::makeColor(AgentParameters::kActBodyR,  AgentParameters::kActBodyG,  AgentParameters::kActBodyB,  AgentParameters::kActBodyA);
        rotor_color = MarkerHelpers::makeColor(AgentParameters::kActRotorR, AgentParameters::kActRotorG, AgentParameters::kActRotorB, AgentParameters::kRotorAlpha);
    }
    else if (agent_.has_status && agent_.status == 2) // ERROR
    {
        body_color  = MarkerHelpers::makeColor(AgentParameters::kErrBodyR,  AgentParameters::kErrBodyG,  AgentParameters::kErrBodyB,  AgentParameters::kErrBodyA);
        rotor_color = MarkerHelpers::makeColor(AgentParameters::kErrRotorR, AgentParameters::kErrRotorG, AgentParameters::kErrRotorB, AgentParameters::kRotorAlpha);
    }
    else // IDLE
    {
        body_color  = MarkerHelpers::makeColor(AgentParameters::kIdleBodyR,  AgentParameters::kIdleBodyG,  AgentParameters::kIdleBodyB,  AgentParameters::kIdleBodyA);
        rotor_color = MarkerHelpers::makeColor(AgentParameters::kIdleRotorR, AgentParameters::kIdleRotorG, AgentParameters::kIdleRotorB, AgentParameters::kRotorAlpha);
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

    // Body origin relative to agent body frame
    PointMsg body_pos;
    body_pos.x = 0.0;
    body_pos.y = 0.0;
    body_pos.z = 0.0;

    // ── 1. Central fuselage disc ───────────────────────────────────────────
    {
        FoxCylinderPrimitiveMsg body;
        body.pose.position = body_pos;
        body.pose.orientation.w = 1.0;
        body.size.x = AgentParameters::kBodyDiamXY;
        body.size.y = AgentParameters::kBodyDiamXY;
        body.size.z = AgentParameters::kBodyDiamZ;
        body.color = body_color;
        body.top_scale = 1.0f;
        body.bottom_scale = 1.0f;
        entity.cylinders.push_back(body);
    }

    // ── 2. Motor arms + rotor discs ─────
    constexpr double kSin45 = 0.7071067811865476;
    constexpr double arm_angles[4] = { M_PI * 0.25, M_PI * 0.75, M_PI * 1.25, M_PI * 1.75 };
    for (double angle : arm_angles)
    {
        // Midpoint of the arm (centre → rotor tip)
        const double half = AgentParameters::kArmLength * 0.5;
        const double cx = half * std::cos(angle);
        const double cy = half * std::sin(angle);
        const double rx = AgentParameters::kArmLength * std::cos(angle);
        const double ry = AgentParameters::kArmLength * std::sin(angle);

        // Quaternion rotating +Z onto arm direction
        const double qx = -std::sin(angle) * kSin45;
        const double qy =  std::cos(angle) * kSin45;

        // Arm cylinder: spans from fuselage centre to rotor mount
        FoxCylinderPrimitiveMsg arm;
        arm.pose.position.x = cx;
        arm.pose.position.y = cy;
        arm.pose.position.z = 0.0;
        arm.pose.orientation.x = qx;
        arm.pose.orientation.y = qy;
        arm.pose.orientation.z = 0.0;
        arm.pose.orientation.w = kSin45;
        arm.size.x = AgentParameters::kArmDiam;
        arm.size.y = AgentParameters::kArmDiam;
        arm.size.z = AgentParameters::kArmLength;
        arm.color = rotor_color;
        arm.top_scale = 1.0f;
        arm.bottom_scale = 1.0f;
        entity.cylinders.push_back(arm);

        // Rotor disc: large, flat, semi-transparent cylinder at arm tip
        FoxCylinderPrimitiveMsg rotor;
        rotor.pose.position.x = rx;
        rotor.pose.position.y = ry;
        rotor.pose.position.z = 0.0;
        rotor.pose.orientation.w = 1.0;
        rotor.size.x = AgentParameters::kRotorDiam;
        rotor.size.y = AgentParameters::kRotorDiam;
        rotor.size.z = AgentParameters::kRotorThickness;
        rotor.color = rotor_color;
        rotor.top_scale = 1.0f;
        rotor.bottom_scale = 1.0f;
        entity.cylinders.push_back(rotor);
    }

    // ── 3. Heading arrow ───────────────────────────
    if (AgentParameters::kShowArrow)
    {
        FoxArrowPrimitiveMsg arrow;
        arrow.pose.position = body_pos;
        // Default arrow primitive points along +X — no rotation needed
        arrow.pose.orientation.w = 1.0;
        arrow.shaft_length   = AgentParameters::kArrowShaftLen;
        arrow.shaft_diameter = AgentParameters::kArrowShaftDiam;
        arrow.head_length    = AgentParameters::kArrowHeadLen;
        arrow.head_diameter  = AgentParameters::kArrowHeadDiam;
        arrow.color = body_color;
        entity.arrows.push_back(arrow);
    }

    // ── 4. Text label ─────────────────────────────────────────────────────
    if (AgentParameters::kDisplayText)
    {
        FoxTextPrimitiveMsg text;
        text.pose.position = body_pos;
        text.pose.position.x += AgentParameters::kLabelXOffset;
        text.pose.position.y += AgentParameters::kLabelYOffset;
        text.pose.position.z += AgentParameters::kLabelZOffset;
        text.pose.orientation.w = 1.0;
        text.billboard = true;
        text.font_size = AgentParameters::kFontSize;
        text.scale_invariant = false;
        text.color = MarkerHelpers::white();
        text.text = agent_id_ + "\nh=" + oss.str() + " m";
        entity.texts.push_back(text);
    }

    // ── 5. Setpoint ─────────────────────────────────────────────────────
    if (AgentParameters::kShowSetpoint && agent_.has_setpoint)
    {
        FoxSceneEntityMsg setpoint_entity;
        MarkerHelpers::stampEntity(setpoint_entity, stamp, lifetime);
        setpoint_entity.frame_id = global_frame;
        setpoint_entity.id = agent_id_ + "_setpoint";

        // 1. Setpoint Sphere
        {
            FoxSpherePrimitiveMsg sphere;
            sphere.pose.position = agent_.setpoint;
            sphere.pose.orientation.w = 1.0;
            sphere.size.x = AgentParameters::kSetpointDiam;
            sphere.size.y = AgentParameters::kSetpointDiam;
            sphere.size.z = AgentParameters::kSetpointDiam;
            sphere.color = MarkerHelpers::makeColor(
                AgentParameters::kSetpointR,
                AgentParameters::kSetpointG,
                AgentParameters::kSetpointB,
                AgentParameters::kSetpointA
            );
            setpoint_entity.spheres.push_back(sphere);
        }

        // 2. Connecting line from drone to setpoint
        {
            FoxLinePrimitiveMsg line;
            line.type = FoxLinePrimitiveMsg::LINE_STRIP;
            line.pose.orientation.w = 1.0;
            line.thickness = AgentParameters::kSetpointLineThickness;
            line.color = MarkerHelpers::makeColor(
                AgentParameters::kSetpointLineR,
                AgentParameters::kSetpointLineG,
                AgentParameters::kSetpointLineB,
                AgentParameters::kSetpointLineA
            );
            line.points.push_back(pos);
            line.points.push_back(agent_.setpoint);
            setpoint_entity.lines.push_back(line);
        }

        update_msg.entities.push_back(setpoint_entity);
    }

    // ── Publish SceneUpdate ────────────────────────────────────────────────
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