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
    // Resolve original camera resolution from the central camera config in settings
    original_view_width_  = 1920;
    original_view_height_ = 1080;
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

    // Build element_id from AGENT_IDX environment variable
    const int agent_idx = std::getenv("AGENT_IDX") ? std::stoi(std::getenv("AGENT_IDX")) : 0;
    std::ostringstream element_ss;
    element_ss << "ELEMENT" << std::setw(2) << std::setfill('0') << agent_idx;
    element_id_ = element_ss.str();

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
        ? AnnotationHelpers::makeColor(CameraAnnotations::kCentralR, CameraAnnotations::kCentralG, CameraAnnotations::kCentralB, CameraAnnotations::kCentralA)
        : AnnotationHelpers::makeColor(CameraAnnotations::kTrackR,   CameraAnnotations::kTrackG,   CameraAnnotations::kTrackB,   CameraAnnotations::kTrackA);
    const FoxColorMsg white  = AnnotationHelpers::makeColor(CameraAnnotations::kWhiteR, CameraAnnotations::kWhiteG, CameraAnnotations::kWhiteB, CameraAnnotations::kWhiteA);
    const FoxColorMsg bg     = AnnotationHelpers::makeColor(CameraAnnotations::kBgR,    CameraAnnotations::kBgG,    CameraAnnotations::kBgB,    CameraAnnotations::kBgA);

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
                ? AnnotationHelpers::makeColor(CameraAnnotations::kWinOobR, CameraAnnotations::kWinOobG, CameraAnnotations::kWinOobB, CameraAnnotations::kWinOobA)
                : AnnotationHelpers::makeColor(CameraAnnotations::kWinR,    CameraAnnotations::kWinG,    CameraAnnotations::kWinB,    CameraAnnotations::kWinA);

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

            // Corner ticks
            {
                const float tick = std::min(wx1 - wx0, wy1 - wy0)
                                   * CameraAnnotations::kWinOverlayTickFrac;
                const float t = CameraAnnotations::kWinOverlayTickThick;
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx0, wy0 + tick), AnnotationHelpers::pt(wx0, wy0), win_color, t);
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx0, wy0), AnnotationHelpers::pt(wx0 + tick, wy0), win_color, t);
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx1 - tick, wy0), AnnotationHelpers::pt(wx1, wy0), win_color, t);
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx1, wy0), AnnotationHelpers::pt(wx1, wy0 + tick), win_color, t);
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx1, wy1 - tick), AnnotationHelpers::pt(wx1, wy1), win_color, t);
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx1, wy1), AnnotationHelpers::pt(wx1 - tick, wy1), win_color, t);
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx0 + tick, wy1), AnnotationHelpers::pt(wx0, wy1), win_color, t);
                AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(wx0, wy1), AnnotationHelpers::pt(wx0, wy1 - tick), win_color, t);
            }

            // Unit ID label (top-left of crop box)
            {
                const std::string unit_id = j < sp.ids.size() ? sp.ids[j] : "?";
                FoxTextAnnotationMsg label;
                label.timestamp = stamp;
                label.position.x = wx0 + 3.0f;
                label.position.y = wy0 - CameraAnnotations::kWinOverlayIdFontSz;
                label.text = unit_id;
                label.font_size = CameraAnnotations::kWinOverlayIdFontSz;
                label.text_color = win_color;
                label.background_color = AnnotationHelpers::makeColor(
                    CameraAnnotations::kBgR, CameraAnnotations::kBgG,
                    CameraAnnotations::kBgB, CameraAnnotations::kBgA);
                msg.texts.push_back(label);
            }
        }
    }

    // ── Corner brackets (L-shape at each image corner) ────────────────────
    if (CameraAnnotations::kShowBrackets)
    {
        const float b = side * CameraAnnotations::kBracketFrac;
        const float t = CameraAnnotations::kBracketThick;
        // Top-left
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(0, b),    AnnotationHelpers::pt(0, 0),    white, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(0, 0),    AnnotationHelpers::pt(b, 0),    white, t);
        // Top-right
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(W - b, 0), AnnotationHelpers::pt(W, 0),   white, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(W, 0),    AnnotationHelpers::pt(W, b),    white, t);
        // Bottom-right
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(W, H - b), AnnotationHelpers::pt(W, H),   white, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(W, H),    AnnotationHelpers::pt(W - b, H),white, t);
        // Bottom-left
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(b, H),    AnnotationHelpers::pt(0, H),    white, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(0, H),    AnnotationHelpers::pt(0, H - b),white, t);
    }

    // ── Role / unit ID badge (top-left) ───────────────────────────────────
    if (CameraAnnotations::kShowBadge)
    {
        const std::string unit_id = idx < sp.ids.size() ? sp.ids[idx] : "?";
        const std::string role_str = is_central ? "CENTRAL" : "TRACKING";

        FoxTextAnnotationMsg badge;
        badge.timestamp = stamp;
        badge.position.x = CameraAnnotations::kBadgeMarginX;
        badge.position.y = CameraAnnotations::kBadgeMarginY;
        badge.text = role_str + "  " + unit_id;
        badge.font_size = CameraAnnotations::kBadgeFontSize;
        badge.text_color = hud_color;
        badge.background_color = bg;
        msg.texts.push_back(badge);
    }

    // ── Zoom bar (above HUD text, bottom-left) ────────────────────────────
    const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;
    if (CameraAnnotations::kShowZoomBar)
    {
        const float bx = CameraAnnotations::kZoomBarMarginX;
        const float by = H - CameraAnnotations::kZoomBarMarginY;
        const float fill = std::min(1.0f, zoom / CameraAnnotations::kZoomMax) * CameraAnnotations::kZoomBarW;
        // Background track
        {
            FoxPointsAnnotationMsg track;
            track.type = FoxPointsAnnotationMsg::LINE_LOOP;
            track.timestamp = stamp;
            track.points = {
                AnnotationHelpers::pt(bx,              by),
                AnnotationHelpers::pt(bx + CameraAnnotations::kZoomBarW, by),
                AnnotationHelpers::pt(bx + CameraAnnotations::kZoomBarW, by + CameraAnnotations::kZoomBarH),
                AnnotationHelpers::pt(bx,              by + CameraAnnotations::kZoomBarH)
            };
            track.outline_color = AnnotationHelpers::makeColor(0.3f, 0.3f, 0.3f, 0.5f);
            track.thickness = 1.0f;
            msg.points.push_back(track);
        }
        // Fill bar
        if (fill > 0.0f)
        {
            FoxPointsAnnotationMsg bar;
            bar.type = FoxPointsAnnotationMsg::LINE_LOOP;
            bar.timestamp = stamp;
            bar.points = {
                AnnotationHelpers::pt(bx,         by),
                AnnotationHelpers::pt(bx + fill,  by),
                AnnotationHelpers::pt(bx + fill,  by + CameraAnnotations::kZoomBarH),
                AnnotationHelpers::pt(bx,         by + CameraAnnotations::kZoomBarH)
            };
            bar.outline_color = hud_color;
            bar.thickness = CameraAnnotations::kZoomBarH;
            msg.points.push_back(bar);
        }
    }

    // ── HUD text (zoom + yaw + pitch, bottom-left) ────────────────────────
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

    // The window view IS the extracted crop — coordinates are in display space (0,0)→(view_w,view_h)
    const float x0   = 0.0f;
    const float y0   = 0.0f;
    const float x1   = static_cast<float>(view_w);
    const float y1   = static_cast<float>(view_h);
    const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;
    const bool  oob  = crop.is_out_of_bounds;

    const FoxColorMsg box_color  = oob
        ? AnnotationHelpers::makeColor(WindowAnnotations::kOobBoxR, WindowAnnotations::kOobBoxG, WindowAnnotations::kOobBoxB, WindowAnnotations::kOobBoxA)
        : AnnotationHelpers::makeColor(WindowAnnotations::kBoxR,    WindowAnnotations::kBoxG,    WindowAnnotations::kBoxB,    WindowAnnotations::kBoxA);
    const FoxColorMsg text_color = oob
        ? AnnotationHelpers::makeColor(WindowAnnotations::kOobTxtR, WindowAnnotations::kOobTxtG, WindowAnnotations::kOobTxtB, WindowAnnotations::kOobTxtA)
        : AnnotationHelpers::makeColor(WindowAnnotations::kTextR,   WindowAnnotations::kTextG,   WindowAnnotations::kTextB,   WindowAnnotations::kTextA);
    const FoxColorMsg tick_color = AnnotationHelpers::makeColor(WindowAnnotations::kTickR, WindowAnnotations::kTickG, WindowAnnotations::kTickB, WindowAnnotations::kTickA);
    const FoxColorMsg bg         = AnnotationHelpers::makeColor(WindowAnnotations::kBgR,   WindowAnnotations::kBgG,   WindowAnnotations::kBgB,   WindowAnnotations::kBgA);

    // ── Role / unit ID badge (top-left of the window view) ───────────────
    if (WindowAnnotations::kShowBadge)
    {
        const std::string unit_id = idx < sp.ids.size() ? sp.ids[idx] : "?";
        FoxTextAnnotationMsg badge;
        badge.timestamp = stamp;
        badge.position.x = WindowAnnotations::kBadgeMarginX;
        badge.position.y = WindowAnnotations::kBadgeMarginY;
        badge.text = "WINDOW  " + unit_id + (oob ? "  [OOB]" : "");
        badge.font_size = WindowAnnotations::kBadgeFontSize;
        badge.text_color = text_color;
        badge.background_color = bg;
        msg.texts.push_back(badge);
    }

    // ── Bounding box (LINE_LOOP) ───────────────────────────────────────────
    {
        FoxPointsAnnotationMsg rect;
        rect.type = FoxPointsAnnotationMsg::LINE_LOOP;
        rect.timestamp = stamp;
        rect.points = { AnnotationHelpers::pt(x0, y0), AnnotationHelpers::pt(x1, y0), AnnotationHelpers::pt(x1, y1), AnnotationHelpers::pt(x0, y1) };
        rect.outline_color = box_color;
        rect.thickness = WindowAnnotations::kBoxThick;
        msg.points.push_back(rect);
    }

    // ── Corner ticks (L-shape at each corner) ─────────────────────────────
    {
        const float tick = std::min(static_cast<float>(crop.w), static_cast<float>(crop.h)) * WindowAnnotations::kTickFrac;
        const float t    = WindowAnnotations::kTickThick;
        // Top-left
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x0, y0 + tick), AnnotationHelpers::pt(x0, y0),        tick_color, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x0, y0),        AnnotationHelpers::pt(x0 + tick, y0), tick_color, t);
        // Top-right
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x1 - tick, y0), AnnotationHelpers::pt(x1, y0),        tick_color, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x1, y0),        AnnotationHelpers::pt(x1, y0 + tick), tick_color, t);
        // Bottom-right
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x1, y1 - tick), AnnotationHelpers::pt(x1, y1),        tick_color, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x1, y1),        AnnotationHelpers::pt(x1 - tick, y1), tick_color, t);
        // Bottom-left
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x0 + tick, y1), AnnotationHelpers::pt(x0, y1),        tick_color, t);
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(x0, y1),        AnnotationHelpers::pt(x0, y1 - tick), tick_color, t);
    }

    // ── Info text (crop size + zoom, bottom-left of view) ────────────────
    {
        std::ostringstream oss;
        oss << crop.w << "x" << crop.h
            << "  z=" << std::fixed << std::setprecision(2) << zoom;

        FoxTextAnnotationMsg info;
        info.timestamp = stamp;
        info.position.x = x0 + 3.0f;
        info.position.y = y1 - WindowAnnotations::kInfoOffsetY;
        info.text = oss.str();
        info.font_size = WindowAnnotations::kInfoFontSize;
        info.text_color = text_color;
        info.background_color = bg;
        msg.texts.push_back(info);
    }

    // ── Zoom fill bar (just below box bottom edge) ────────────────────────
    if (WindowAnnotations::kShowZoomBar)
    {
        const float bx    = x0;
        const float by    = y1 - WindowAnnotations::kZoomBarMarginY;
        const float maxW  = x1 - x0;
        const float fill  = std::min(1.0f, zoom / WindowAnnotations::kZoomMax) * maxW;
        // Background track
        {
            FoxPointsAnnotationMsg track;
            track.type = FoxPointsAnnotationMsg::LINE_LOOP;
            track.timestamp = stamp;
            track.points = {
                AnnotationHelpers::pt(bx,        by),
                AnnotationHelpers::pt(bx + maxW, by),
                AnnotationHelpers::pt(bx + maxW, by + WindowAnnotations::kZoomBarH),
                AnnotationHelpers::pt(bx,        by + WindowAnnotations::kZoomBarH)
            };
            track.outline_color = AnnotationHelpers::makeColor(WindowAnnotations::kZoomBgR, WindowAnnotations::kZoomBgG, WindowAnnotations::kZoomBgB, WindowAnnotations::kZoomBgA);
            track.thickness = 1.0f;
            msg.points.push_back(track);
        }
        // Fill
        if (fill > 0.0f)
        {
            FoxPointsAnnotationMsg bar;
            bar.type = FoxPointsAnnotationMsg::LINE_LOOP;
            bar.timestamp = stamp;
            bar.points = {
                AnnotationHelpers::pt(bx,        by),
                AnnotationHelpers::pt(bx + fill, by),
                AnnotationHelpers::pt(bx + fill, by + WindowAnnotations::kZoomBarH),
                AnnotationHelpers::pt(bx,        by + WindowAnnotations::kZoomBarH)
            };
            bar.outline_color = AnnotationHelpers::makeColor(WindowAnnotations::kZoomBarR, WindowAnnotations::kZoomBarG, WindowAnnotations::kZoomBarB, WindowAnnotations::kZoomBarA);
            bar.thickness = WindowAnnotations::kZoomBarH;
            msg.points.push_back(bar);
        }
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