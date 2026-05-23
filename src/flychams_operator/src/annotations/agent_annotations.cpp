#include "flychams_operator/annotations/agent_annotations.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("annotation_rate", 10.0f);
    // View dimensions
    central_view_width_   = node_->getParameterOr<int>("central_view.width", 854);
    central_view_height_  = node_->getParameterOr<int>("central_view.height", 480);
    tracking_view_width_  = node_->getParameterOr<int>("tracking_view.width", 427);
    tracking_view_height_ = node_->getParameterOr<int>("tracking_view.height", 240);

    // Build element_id from AGENT_IDX environment variable
    const int agent_idx = std::getenv("AGENT_IDX") ? std::stoi(std::getenv("AGENT_IDX")) : 0;
    std::ostringstream element_ss;
    element_ss << "ELEMENT" << std::setw(2) << std::setfill('0') << agent_idx;
    element_id_ = element_ss.str();

    // Get central camera resolution from settings
    const auto& tracking_config = node_->getSettings()->getTracking(agent_id_);
    for (const auto& [camera_id, camera] : tracking_config.multi_camera_set)
    {
        if (camera->role == ObservationRole::Central)
        {
            original_view_width_  = camera->camera.resolution(0);
            original_view_height_ = camera->camera.resolution(1);
            break;
        }
    }

    // Subscriber
    setpoints_sub_ = node_->createObservationSetpointsSubscriber(agent_id_,
        std::bind(&AgentAnnotations::observationSetpointsCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentAnnotations::update, this));
}

void AgentAnnotations::onModuleShutdown()
{
    update_timer_.reset();
    annotation_pubs_.clear();
    setpoints_sub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::observationSetpointsCallback(const ObservationSetpointsMsg::SharedPtr msg)
{
    setpoints_ = msg;
    has_setpoints_ = true;

    // Lazily create publishers for each observation unit if needed
    const size_t n = msg->ids.size();
    if (annotation_pubs_.size() != n)
    {
        annotation_pubs_.clear();
        annotation_pubs_.reserve(n);
        for (size_t i = 0; i < n; ++i)
        {
            std::ostringstream view_ss;
            view_ss << "VIEW" << std::setw(2) << std::setfill('0') << i;
            annotation_pubs_.push_back(
                node_->createAnnotationsPublisher(element_id_, view_ss.str()));
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::update()
{
    if (!isDataValid())
    {
        return;
    }

    const auto& sp = *setpoints_;
    const size_t n = sp.ids.size();

    for (size_t i = 0; i < n && i < annotation_pubs_.size(); ++i)
    {
        const uint8_t type = i < sp.types.size() ? sp.types[i] : 0;
        const uint8_t role = i < sp.roles.size() ? sp.roles[i] : 0;

        // Central camera → full resolution view, tracking units → half resolution
        const bool is_central = (role == 1 /* Central */);
        const int view_w = is_central ? central_view_width_  : tracking_view_width_;
        const int view_h = is_central ? central_view_height_ : tracking_view_height_;

        if (type == 1 /* Camera */)
        {
            publishCameraAnnotations(i, view_w, view_h);
        }
        else if (type == 2 /* Window */)
        {
            publishWindowAnnotations(i, view_w, view_h);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// ANNOTATION HELPERS
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::publishCameraAnnotations(size_t idx, int view_w, int view_h) const
{
    const auto& sp = *setpoints_;
    const auto stamp = node_->now();

    // Get zoom factor (focal)
    const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;

    FoxImageAnnotationsMsg msg;

    const float W  = static_cast<float>(view_w);
    const float H  = static_cast<float>(view_h);
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;
    const float side = std::min(W, H);

    // Role-based color (Central = cyan, Tracking = amber)
    const uint8_t role = idx < sp.roles.size() ? sp.roles[idx] : 0;
    const bool is_central = (role == 1);
    const FoxColorMsg hud_color = is_central
        ? AnnotationHelpers::makeColor(CameraAnnotations::kCentral)
        : AnnotationHelpers::makeColor(CameraAnnotations::kTrack);
    const FoxColorMsg bg     = AnnotationHelpers::makeColor(CameraAnnotations::kBg);
    const FoxColorMsg white  = AnnotationHelpers::makeColor(Colors::kWhite);

    // ── Crosshair with centre gap ──────────────────────────────────────────
    {
        const float arm = side * CameraAnnotations::kCrosshairArmFrac;
        const float gap = side * CameraAnnotations::kCrosshairGapFrac;
        // Horizontal: left segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx - arm, cy), AnnotationHelpers::pt(cx - gap, cy), hud_color, CameraAnnotations::kCrosshairThick);
        // Horizontal: right segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx + gap, cy), AnnotationHelpers::pt(cx + arm, cy), hud_color, CameraAnnotations::kCrosshairThick);
        // Vertical: top segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy - arm), AnnotationHelpers::pt(cx, cy - gap), hud_color, CameraAnnotations::kCrosshairThick);
        // Vertical: bottom segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy + gap), AnnotationHelpers::pt(cx, cy + arm), hud_color, CameraAnnotations::kCrosshairThick);
    }

    // ── Centre dot ────────────────────────────────────────────────────────
    {
        FoxCircleAnnotationMsg dot;
        dot.timestamp = stamp;
        dot.position.x = cx;
        dot.position.y = cy;
        dot.diameter = CameraAnnotations::kCentreDotDiam;
        dot.fill_color = hud_color;
        dot.outline_color = white;
        dot.thickness = 1.0f;
        msg.circles.push_back(dot);
    }

    // ── Window crop overlays (central view only) ──────────────────────────
    if (is_central && CameraAnnotations::kShowWindowsOnCentral)
    {
        const size_t n = sp.ids.size();
        for (size_t j = 0; j < n; ++j)
        {
            const uint8_t wtype = j < sp.types.size() ? sp.types[j] : 0;
            if (wtype != 2 /* Window */ || j >= sp.crops.size())
            {
                continue;
            }

            const auto& crop = sp.crops[j];
            const float sx  = static_cast<float>(view_w) / static_cast<float>(original_view_width_);
            const float sy  = static_cast<float>(view_h) / static_cast<float>(original_view_height_);
            const float wx0 = static_cast<float>(crop.x)           * sx;
            const float wy0 = static_cast<float>(crop.y)           * sy;
            const float wx1 = static_cast<float>(crop.x + crop.w)  * sx;
            const float wy1 = static_cast<float>(crop.y + crop.h)  * sy;

            const FoxColorMsg win_color = crop.is_out_of_bounds
                ? AnnotationHelpers::makeColor(CameraAnnotations::kWinOob)
                : AnnotationHelpers::makeColor(CameraAnnotations::kWin);

            // Box
            {
                FoxPointsAnnotationMsg rect;
                rect.type = FoxPointsAnnotationMsg::LINE_LOOP;
                rect.timestamp = stamp;
                rect.points = {
                    AnnotationHelpers::pt(wx0, wy0), AnnotationHelpers::pt(wx1, wy0),
                    AnnotationHelpers::pt(wx1, wy1), AnnotationHelpers::pt(wx0, wy1)
                };
                rect.outline_color = win_color;
                rect.thickness = CameraAnnotations::kWinOverlayBoxThick;
                msg.points.push_back(rect);
            }

            // Unit ID label (top-left of crop box)
            {
                const std::string unit_id = j < sp.ids.size() ? sp.ids[j] : "?";
                FoxTextAnnotationMsg label;
                label.timestamp = stamp;
                label.position.x = wx0 + CameraAnnotations::kWinOverlayIdFontMarginX;
                label.position.y = wy0 + CameraAnnotations::kWinOverlayIdFontMarginY;
                label.text = unit_id;
                label.font_size = CameraAnnotations::kWinOverlayIdFontSz;
                label.text_color = win_color;
                label.background_color = AnnotationHelpers::makeColor(CameraAnnotations::kBg);
                msg.texts.push_back(label);
            }
        }
    }

    // ── Role / unit ID badge ───────────────────────────────────
    if (CameraAnnotations::kShowBadge)
    {
        const std::string unit_id = idx < sp.ids.size() ? sp.ids[idx] : "?";
        const std::string role_str = is_central ? "CENTRAL" : "TRACKING";

        FoxTextAnnotationMsg badge;
        badge.timestamp = stamp;
        badge.position.x = CameraAnnotations::kBadgeMarginX;
        badge.position.y = CameraAnnotations::kBadgeMarginY;
        badge.text = role_str + " - " + unit_id;
        badge.font_size = CameraAnnotations::kBadgeFontSize;
        badge.text_color = hud_color;
        badge.background_color = bg;
        msg.texts.push_back(badge);
    }

    // ── HUD text ────────────────────────
    if (CameraAnnotations::kShowHud)
    {
        std::ostringstream oss;
        oss << std::fixed;
        oss << "z=" << std::setprecision(2) << zoom;
        if (idx < sp.rotations.size())
        {
            constexpr float kR2D = 180.0f / static_cast<float>(M_PI);
            const float pitch_deg = sp.rotations[idx].y * kR2D;
            const float yaw_deg   = sp.rotations[idx].z * kR2D;
            oss << "  p=" << std::setprecision(1) << pitch_deg << "\xC2\xB0"
                << "  y=" << yaw_deg << "\xC2\xB0";
        }

        FoxTextAnnotationMsg hud;
        hud.timestamp = stamp;
        hud.position.x = CameraAnnotations::kHudMarginX;
        hud.position.y = H - CameraAnnotations::kHudMarginY;
        hud.text = oss.str();
        hud.font_size = CameraAnnotations::kHudFontSize;
        hud.text_color = hud_color;
        hud.background_color = bg;
        msg.texts.push_back(hud);
    }

    annotation_pubs_[idx]->publish(msg);
}

void AgentAnnotations::publishWindowAnnotations(size_t idx, int view_w, int view_h) const
{
    const auto& sp = *setpoints_;
    const auto stamp = node_->now();

    if (idx >= sp.crops.size())
    {
        return;
    }

    const auto& crop = sp.crops[idx];
    FoxImageAnnotationsMsg msg;

    const float y1   = static_cast<float>(view_h);
    const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;
    const bool  oob  = crop.is_out_of_bounds;

    const FoxColorMsg text_color = oob
        ? AnnotationHelpers::makeColor(WindowAnnotations::kOobText)
        : AnnotationHelpers::makeColor(WindowAnnotations::kText);
    const FoxColorMsg bg         = AnnotationHelpers::makeColor(WindowAnnotations::kBg);

    // ── Role / unit ID badge (top-left of the window view) ───────────────
    if (WindowAnnotations::kShowBadge)
    {
        const std::string unit_id = idx < sp.ids.size() ? sp.ids[idx] : "?";
        FoxTextAnnotationMsg badge;
        badge.timestamp = stamp;
        badge.position.x = WindowAnnotations::kBadgeMarginX;
        badge.position.y = WindowAnnotations::kBadgeMarginY;
        badge.text = "WINDOW - " + unit_id + (oob ? "  [OOB]" : "");
        badge.font_size = WindowAnnotations::kBadgeFontSize;
        badge.text_color = text_color;
        badge.background_color = bg;
        msg.texts.push_back(badge);
    }

    // ── HUD text (crop size + zoom, bottom-left corner)
    if (WindowAnnotations::kShowHud)
    {
        std::ostringstream oss;
        oss << crop.w << "x" << crop.h
            << "  z=" << std::fixed << std::setprecision(2) << zoom;

        FoxTextAnnotationMsg hud;
        hud.timestamp = stamp;
        hud.position.x = WindowAnnotations::kHudMarginX;
        hud.position.y = y1 - WindowAnnotations::kHudMarginY;
        hud.text = oss.str();
        hud.font_size = WindowAnnotations::kHudFontSize;
        hud.text_color = text_color;
        hud.background_color = bg;
        msg.texts.push_back(hud);
    }

    annotation_pubs_[idx]->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS CHECK
// ════════════════════════════════════════════════════════════════════════════

bool AgentAnnotations::isDataValid() const
{
    if (!has_setpoints_)
    {
        return false;
    }
    return true;
}