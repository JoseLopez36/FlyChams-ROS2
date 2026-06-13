#include "flychams_operator/annotations/view_hud.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// PUBLIC DRAW
// ════════════════════════════════════════════════════════════════════════════

void ViewHud::drawCamera(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const CameraSpec& spec)
{
    if (!HudAnnotations::kEnabled)
        return;

    const float W     = static_cast<float>(spec.layout.view_w);
    const float H     = static_cast<float>(spec.layout.view_h);
    const float cx    = W * 0.5f;
    const float cy    = H * 0.5f;
    const float side  = std::min(W, H);
    const float scale = spec.layout.scale;

    const FoxColorMsg accent = AnnotationHelpers::makeColor(withAlpha(AgentColors::get(spec.agent_idx), 0.70f));
    const FoxColorMsg white  = AnnotationHelpers::makeColor(withAlpha(Colors::kWhite, 0.70f));

    // Geometry first (points / circles), then all bar labels last so text stays on top
    drawCrosshair(msg, stamp, cx, cy, side, scale, accent, white);
    drawHeaderBar(msg, stamp, W, scale, accent);
    drawFooterBar(msg, stamp, W, H, scale, accent);

    const Role role = spec.role;
    drawHeaderLabels(msg, stamp, W, scale, accent,
        spec.agent_id,
        roleLabel(role),
        spec.unit_id);
    drawFooterCameraLabels(msg, stamp, W, H, scale, accent,
        spec.upsilon_norm,
        spec.pitch_deg,
        spec.yaw_deg,
        spec.has_rotation);
}

void ViewHud::drawWindow(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const WindowSpec& spec)
{
    if (!HudAnnotations::kEnabled)
        return;

    const float W     = static_cast<float>(spec.layout.view_w);
    const float H     = static_cast<float>(spec.layout.view_h);
    const float cx    = W * 0.5f;
    const float cy    = H * 0.5f;
    const float side  = std::min(W, H);
    const float scale = spec.layout.scale;

    const FoxColorMsg accent = AnnotationHelpers::makeColor(withAlpha(AgentColors::get(spec.agent_idx), 0.70f));
    const FoxColorMsg white  = AnnotationHelpers::makeColor(withAlpha(Colors::kWhite, 0.70f));

    drawCrosshair(msg, stamp, cx, cy, side, scale, accent, white);
    drawHeaderBar(msg, stamp, W, scale, accent);
    drawFooterBar(msg, stamp, W, H, scale, accent);

    std::string role_text = roleLabel(Role::Window);
    if (spec.out_of_bounds)
        role_text += "  OOB";

    drawHeaderLabels(msg, stamp, W, scale, accent,
        "CROP",
        role_text,
        spec.unit_id,
        spec.out_of_bounds);
    drawFooterWindowLabels(msg, stamp, W, H, scale, accent,
        spec.upsilon_norm,
        spec.crop_w,
        spec.crop_h);
}

// ════════════════════════════════════════════════════════════════════════════
// PRIMITIVES
// ════════════════════════════════════════════════════════════════════════════

void ViewHud::drawCrosshair(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float cx, float cy, float side, float scale, const FoxColorMsg& accent, const FoxColorMsg& white)
{
    const float arm   = side * HudAnnotations::kCrosshairArmFrac;
    const float gap   = side * HudAnnotations::kCrosshairGapFrac;
    const float thick = HudAnnotations::kCrosshairThick * scale;
    const FoxColorMsg col = AnnotationHelpers::makeColor(HudAnnotations::kCross);

    // Outer guide (col)
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx - arm, cy), AnnotationHelpers::pt(cx - gap, cy), col, thick * 0.65f);
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx + gap, cy), AnnotationHelpers::pt(cx + arm, cy), col, thick * 0.65f);
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy - arm), AnnotationHelpers::pt(cx, cy - gap), col, thick * 0.65f);
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy + gap), AnnotationHelpers::pt(cx, cy + arm), col, thick * 0.65f);

    // Primary reticle (accent)
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx - arm, cy), AnnotationHelpers::pt(cx - gap, cy), accent, thick);
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx + gap, cy), AnnotationHelpers::pt(cx + arm, cy), accent, thick);
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy - arm), AnnotationHelpers::pt(cx, cy - gap), accent, thick);
    AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy + gap), AnnotationHelpers::pt(cx, cy + arm), accent, thick);

    FoxCircleAnnotationMsg dot;
    dot.timestamp = stamp;
    dot.position.x = cx;
    dot.position.y = cy;
    dot.diameter = HudAnnotations::kCentreDotDiam * scale;
    dot.fill_color = accent;
    dot.outline_color = white;
    dot.thickness = HudAnnotations::kCentreDotOutline * scale;
    msg.circles.push_back(dot);
}

void ViewHud::drawHeaderBar(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float scale, const FoxColorMsg& accent)
{
    const float bar_h = HudAnnotations::kBarHeight * scale;
    const FoxColorMsg fill = AnnotationHelpers::makeColor(HudAnnotations::kBarFill);

    AnnotationHelpers::addFilledRect(msg, stamp, 0.0f, 0.0f, W, bar_h, fill);
    AnnotationHelpers::addLine(msg, stamp,
        AnnotationHelpers::pt(0.0f, bar_h),
        AnnotationHelpers::pt(W, bar_h),
        accent,
        HudAnnotations::kBarAccentThick * scale);
}

void ViewHud::drawFooterBar(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float H, float scale, const FoxColorMsg& accent)
{
    const float bar_h = HudAnnotations::kBarHeight * scale;
    const float y0    = H - bar_h;
    const FoxColorMsg fill = AnnotationHelpers::makeColor(HudAnnotations::kBarFill);

    AnnotationHelpers::addFilledRect(msg, stamp, 0.0f, y0, W, H, fill);
    AnnotationHelpers::addLine(msg, stamp,
        AnnotationHelpers::pt(0.0f, y0),
        AnnotationHelpers::pt(W, y0),
        accent,
        HudAnnotations::kBarAccentThick * scale);
}

void ViewHud::drawHeaderLabels(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float scale, const FoxColorMsg& accent, const std::string& left_text, const std::string& role_text, const std::string& right_text, bool role_warn)
{
    const float bar_h    = HudAnnotations::kBarHeight * scale;
    const float pad_x    = HudAnnotations::kBarPadX * scale;
    const float font_sz  = HudAnnotations::kBarFontSize * scale;
    const float role_sz  = HudAnnotations::kBarRoleFontSize * scale;
    const float text_baseline = (bar_h + font_sz) * 0.5f;
    const float role_baseline = (bar_h + role_sz) * 0.5f;
    const FoxColorMsg fill = AnnotationHelpers::makeColor(HudAnnotations::kBarFill);
    const FoxColorMsg col  = AnnotationHelpers::makeColor(HudAnnotations::kBarText);
    const FoxColorMsg role_color = role_warn
        ? AnnotationHelpers::makeColor(withAlpha(Colors::kOrange, HudAnnotations::kRoleTextAlpha))
        : AnnotationHelpers::makeColor(withAlpha(accent, HudAnnotations::kRoleTextAlpha));

    appendBarText(msg, stamp, pad_x, text_baseline, left_text, font_sz, col, fill);

    const std::string badge = "[ " + role_text + " ]";
    appendBarText(msg, stamp,
        W * 0.5f - estimateTextWidth(badge, role_sz) * 0.5f,
        role_baseline,
        badge, role_sz, role_color, fill);

    appendBarTextRight(msg, stamp, W, pad_x, text_baseline, right_text, font_sz, col, fill);
}

void ViewHud::drawFooterCameraLabels(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float H, float scale, const FoxColorMsg& accent, float upsilon_norm, float pitch_deg, float yaw_deg, bool has_rotation)
{
    const float bar_h    = HudAnnotations::kBarHeight * scale;
    const float pad_x    = HudAnnotations::kBarPadX * scale;
    const float y0       = H - bar_h;
    const float font_sz  = HudAnnotations::kBarFontSize * scale;
    const float text_baseline = y0 + (bar_h + font_sz) * 0.5f;
    const FoxColorMsg fill = AnnotationHelpers::makeColor(HudAnnotations::kBarFill);
    const FoxColorMsg col  = AnnotationHelpers::makeColor(HudAnnotations::kBarText);

    std::ostringstream left;
    left << std::fixed << std::setprecision(3) << "ZOOM: " << upsilon_norm;
    appendBarText(msg, stamp, pad_x, text_baseline, left.str(), font_sz, col, fill);

    if (has_rotation)
    {
        std::ostringstream right;
        right << std::fixed << std::showpos << std::setprecision(1);
        right << "P " << std::setw(6) << pitch_deg << "\xC2\xB0"
              << "  Y " << std::setw(6) << yaw_deg << "\xC2\xB0";
        const std::string right_str = right.str();
        appendBarTextRight(msg, stamp, W, pad_x, text_baseline, right_str, font_sz, col, fill);
    }
}

void ViewHud::drawFooterWindowLabels(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float H, float scale, const FoxColorMsg& accent, float upsilon_norm, int crop_w, int crop_h)
{
    const float bar_h    = HudAnnotations::kBarHeight * scale;
    const float pad_x    = HudAnnotations::kBarPadX * scale;
    const float y0       = H - bar_h;
    const float font_sz  = HudAnnotations::kBarFontSize * scale;
    const float text_baseline = y0 + (bar_h + font_sz) * 0.5f;
    const FoxColorMsg fill = AnnotationHelpers::makeColor(HudAnnotations::kBarFill);
    const FoxColorMsg col  = AnnotationHelpers::makeColor(HudAnnotations::kBarText);

    std::ostringstream left;
    left << std::fixed << std::setprecision(3) << "ZOOM: " << upsilon_norm;
    appendBarText(msg, stamp, pad_x, text_baseline, left.str(), font_sz, col, fill);

    std::ostringstream right;
    right << "RES: " << crop_w << "\xC3\x97" << crop_h;
    const std::string right_str = right.str();
    appendBarTextRight(msg, stamp, W, pad_x, text_baseline, right_str, font_sz, col, fill);
}

void ViewHud::appendBarText(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float x, float baseline, const std::string& text, float font_sz, const FoxColorMsg& color, const FoxColorMsg& fill)
{
    FoxTextAnnotationMsg label;
    label.timestamp = stamp;
    label.position.x = x;
    label.position.y = baseline;
    label.text = text;
    label.font_size = font_sz;
    label.text_color = color;
    label.background_color = fill;
    msg.texts.push_back(label);
}

void ViewHud::appendBarTextRight(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float pad_x, float baseline, const std::string& text, float font_sz, const FoxColorMsg& color, const FoxColorMsg& fill)
{
    const float width = estimateTextWidth(text, font_sz) + font_sz * HudAnnotations::kTextWidthRightSlack;
    appendBarText(msg, stamp, W - pad_x - width, baseline, text, font_sz, color, fill);
}

float ViewHud::estimateTextWidth(const std::string& text, float font_sz)
{
    float width = 0.0f;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if ((c & 0xC0) == 0x80)
            continue;

        size_t seq = 1;
        if      ((c & 0xE0) == 0xC0) seq = 2;
        else if ((c & 0xF0) == 0xE0) seq = 3;
        else if ((c & 0xF8) == 0xF0) seq = 4;
        i += seq - 1;

        // UTF-8 symbols that render wider than a monospace cell
        if (seq == 2 && c == 0xC2 && i < text.size() && static_cast<unsigned char>(text[i]) == 0xB0)
        {
            width += HudAnnotations::kTextWidthDegree * font_sz;
            continue;
        }

        float factor = HudAnnotations::kTextWidthFactor;
        if (c == ' ' || c == '.' || c == ':' || c == '+' || c == '-')
            factor *= HudAnnotations::kTextWidthNarrow;
        width += factor * font_sz;
    }
    return width;
}

std::string ViewHud::roleLabel(Role role)
{
    switch (role)
    {
        case Role::Central:  return "CENTRAL";
        case Role::Tracking: return "TRACKING";
        case Role::Window:   return "WINDOW";
        default:             return "UNKNOWN";
    }
}
