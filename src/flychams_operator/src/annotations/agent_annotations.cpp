#include "flychams_operator/annotations/agent_annotations.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

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

    const float cx = static_cast<float>(view_w) * 0.5f;
    const float cy = static_cast<float>(view_h) * 0.5f;
    const float arm = std::min(static_cast<float>(view_w), static_cast<float>(view_h)) * 0.04f;

    // ── Crosshair: horizontal arm ──────────────────────────────────────────
    {
        FoxPointsAnnotationMsg line;
        line.type = FoxPointsAnnotationMsg::LINE_STRIP;
        line.timestamp = stamp;
        FoxPoint2Msg p1; p1.x = cx - arm; p1.y = cy;
        FoxPoint2Msg p2; p2.x = cx + arm; p2.y = cy;
        line.points = {p1, p2};
        FoxColorMsg color;
        color.r = 0.0f; color.g = 0.9f; color.b = 1.0f; color.a = 0.9f;
        line.outline_color = color;
        line.thickness = 1.5f;
        msg.points.push_back(line);
    }

    // ── Crosshair: vertical arm ────────────────────────────────────────────
    {
        FoxPointsAnnotationMsg line;
        line.type = FoxPointsAnnotationMsg::LINE_STRIP;
        line.timestamp = stamp;
        FoxPoint2Msg p1; p1.x = cx; p1.y = cy - arm;
        FoxPoint2Msg p2; p2.x = cx; p2.y = cy + arm;
        line.points = {p1, p2};
        FoxColorMsg color;
        color.r = 0.0f; color.g = 0.9f; color.b = 1.0f; color.a = 0.9f;
        line.outline_color = color;
        line.thickness = 1.5f;
        msg.points.push_back(line);
    }

    // ── Centre dot ────────────────────────────────────────────────────────
    {
        FoxCircleAnnotationMsg dot;
        dot.timestamp = stamp;
        dot.position.x = cx;
        dot.position.y = cy;
        dot.diameter = 6.0f;
        FoxColorMsg fill;
        fill.r = 0.0f; fill.g = 0.9f; fill.b = 1.0f; fill.a = 0.95f;
        dot.fill_color = fill;
        FoxColorMsg outline;
        outline.r = 1.0f; outline.g = 1.0f; outline.b = 1.0f; outline.a = 0.7f;
        dot.outline_color = outline;
        dot.thickness = 1.0f;
        msg.circles.push_back(dot);
    }

    // ── Text: zoom and yaw ────────────────────────────────────────────────
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;
        oss << "z=" << zoom;
        if (idx < sp.rotations.size())
        {
            const float yaw_deg = sp.rotations[idx].z * (180.0f / static_cast<float>(M_PI));
            oss << "  yaw=" << std::setprecision(1) << yaw_deg << "\xC2\xB0";
        }

        FoxTextAnnotationMsg text;
        text.timestamp = stamp;
        text.position.x = 6.0f;
        text.position.y = static_cast<float>(view_h) - 18.0f;
        text.text = oss.str();
        text.font_size = 11.0f;
        FoxColorMsg fg;
        fg.r = 0.0f; fg.g = 0.9f; fg.b = 1.0f; fg.a = 0.95f;
        text.text_color = fg;
        FoxColorMsg bg;
        bg.r = 0.0f; bg.g = 0.0f; bg.b = 0.0f; bg.a = 0.50f;
        text.background_color = bg;
        msg.texts.push_back(text);
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

    // ── Crop rectangle outline ────────────────────────────────────────────
    {
        FoxPointsAnnotationMsg rect;
        rect.type = FoxPointsAnnotationMsg::LINE_LOOP;
        rect.timestamp = stamp;

        FoxPoint2Msg tl; tl.x = static_cast<float>(crop.x);           tl.y = static_cast<float>(crop.y);
        FoxPoint2Msg tr; tr.x = static_cast<float>(crop.x + crop.w);  tr.y = static_cast<float>(crop.y);
        FoxPoint2Msg br; br.x = static_cast<float>(crop.x + crop.w);  br.y = static_cast<float>(crop.y + crop.h);
        FoxPoint2Msg bl; bl.x = static_cast<float>(crop.x);           bl.y = static_cast<float>(crop.y + crop.h);
        rect.points = {tl, tr, br, bl};

        FoxColorMsg color;
        if (crop.is_out_of_bounds)
        {
            color.r = 1.0f; color.g = 0.35f; color.b = 0.0f; color.a = 0.85f;
        }
        else
        {
            color.r = 1.0f; color.g = 0.85f; color.b = 0.0f; color.a = 0.90f;
        }
        rect.outline_color = color;
        rect.thickness = 1.5f;
        msg.points.push_back(rect);
    }

    // ── Corner tick marks ─────────────────────────────────────────────────
    {
        const float tick = std::min(static_cast<float>(crop.w), static_cast<float>(crop.h)) * 0.12f;
        FoxColorMsg tick_color;
        tick_color.r = 1.0f; tick_color.g = 1.0f; tick_color.b = 1.0f; tick_color.a = 0.80f;

        auto make_tick = [&](float ax, float ay, float bx, float by)
        {
            FoxPointsAnnotationMsg t;
            t.type = FoxPointsAnnotationMsg::LINE_STRIP;
            t.timestamp = stamp;
            FoxPoint2Msg pa; pa.x = ax; pa.y = ay;
            FoxPoint2Msg pb; pb.x = bx; pb.y = by;
            t.points = {pa, pb};
            t.outline_color = tick_color;
            t.thickness = 1.5f;
            msg.points.push_back(t);
        };

        const float x0 = static_cast<float>(crop.x);
        const float y0 = static_cast<float>(crop.y);
        const float x1 = static_cast<float>(crop.x + crop.w);
        const float y1 = static_cast<float>(crop.y + crop.h);

        // Top-left corner
        make_tick(x0, y0 + tick, x0, y0); make_tick(x0, y0, x0 + tick, y0);
        // Top-right corner
        make_tick(x1 - tick, y0, x1, y0); make_tick(x1, y0, x1, y0 + tick);
        // Bottom-right corner
        make_tick(x1, y1 - tick, x1, y1); make_tick(x1, y1, x1 - tick, y1);
        // Bottom-left corner
        make_tick(x0 + tick, y1, x0, y1); make_tick(x0, y1, x0, y1 - tick);
    }

    // ── Text: crop size and zoom ───────────────────────────────────────────
    {
        std::ostringstream oss;
        oss << crop.w << "x" << crop.h;
        const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;
        oss << "  z=" << std::fixed << std::setprecision(2) << zoom;
        if (crop.is_out_of_bounds)
        {
            oss << " [OOB]";
        }

        FoxTextAnnotationMsg text;
        text.timestamp = stamp;
        text.position.x = static_cast<float>(crop.x) + 3.0f;
        text.position.y = static_cast<float>(crop.y) - 14.0f;
        text.text = oss.str();
        text.font_size = 10.0f;
        FoxColorMsg fg;
        fg.r = 1.0f; fg.g = 0.85f; fg.b = 0.0f; fg.a = 0.95f;
        text.text_color = fg;
        FoxColorMsg bg;
        bg.r = 0.0f; bg.g = 0.0f; bg.b = 0.0f; bg.a = 0.45f;
        text.background_color = bg;
        msg.texts.push_back(text);
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