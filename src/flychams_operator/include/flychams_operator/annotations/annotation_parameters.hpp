#pragma once

#include "flychams_common/types/ros_types.hpp"
#include "flychams_operator/colors/color_dictionary.hpp"

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // CAMERA ANNOTATION PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace CameraAnnotations
    {
        // Crosshair
        constexpr float kCrosshairArmFrac  = 0.04f;
        constexpr float kCrosshairGapFrac  = 0.012f;
        constexpr float kCrosshairThick    = 2.0f;
        // Centre dot
        constexpr float kCentreDotDiam     = 7.5f;
        // Role badge text
        constexpr bool  kShowBadge         = true;
        constexpr float kBadgeFontSize     = 15.0f;
        constexpr float kBadgeMarginX      = 5.0f;
        constexpr float kBadgeMarginY      = 20.0f;
        // HUD text
        constexpr bool  kShowHud           = true;
        constexpr float kHudFontSize       = 15.0f;
        constexpr float kHudMarginX        = 5.0f;
        constexpr float kHudMarginY        = 7.5f;
        // Window-overlay style
        constexpr bool  kShowWindowsOnCentral = true;
        constexpr float kWinOverlayBoxThick   = 2.0f;
        constexpr float kWinOverlayIdFontSz   = 13.0f;
        constexpr float kWinOverlayIdFontMarginX = -1.0f;
        constexpr float kWinOverlayIdFontMarginY = -5.0f;
        // Colors
        constexpr Color kCentral = { Colors::kCyan.r,        Colors::kCyan.g,        Colors::kCyan.b,        0.95f };
        constexpr Color kTrack   = { Colors::kAmber.r,       Colors::kAmber.g,       Colors::kAmber.b,       0.95f };
        constexpr Color kBg      = { Colors::kBlack.r,       Colors::kBlack.g,       Colors::kBlack.b,       0.50f };
        constexpr Color kWin     = { Colors::kCyan.r,        Colors::kCyan.g,        Colors::kCyan.b,        0.80f };
        constexpr Color kWinOob  = { Colors::kScarlettRed.r, Colors::kScarlettRed.g, Colors::kScarlettRed.b, 0.80f };
    }

    // ════════════════════════════════════════════════════════════════════════════
    // WINDOW ANNOTATION PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace WindowAnnotations
    {
        // Role badge text (top-left corner)
        constexpr bool  kShowBadge      = true;
        constexpr float kBadgeFontSize  = 15.0f;
        constexpr float kBadgeMarginX   = 5.0f;
        constexpr float kBadgeMarginY   = 20.0f;
        // Info text (bottom-left corner)
        constexpr bool  kShowHud        = true;
        constexpr float kHudFontSize    = 15.0f;
        constexpr float kHudMarginX     = 5.0f;
        constexpr float kHudMarginY     = 7.5f;
        // Colors
        constexpr Color kText    = { Colors::kCyan.r,        Colors::kCyan.g,        Colors::kCyan.b,        0.95f };
        constexpr Color kOobText = { Colors::kOrange.r,      Colors::kOrange.g,      Colors::kOrange.b,      0.95f };
        constexpr Color kBg      = { Colors::kBlack.r,       Colors::kBlack.g,       Colors::kBlack.b,       0.50f };
    }

    // ════════════════════════════════════════════════════════════════════════════
    // ANNOTATION HELPERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace AnnotationHelpers
    {
        inline void addLine(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, common::FoxPoint2Msg p1, common::FoxPoint2Msg p2, common::FoxColorMsg color, float thickness)
        {
            common::FoxPointsAnnotationMsg line;
            line.type = common::FoxPointsAnnotationMsg::LINE_STRIP;
            line.timestamp = stamp;
            line.points = {p1, p2};
            line.outline_color = color;
            line.thickness = thickness;
            msg.points.push_back(line);
        }

        inline common::FoxPoint2Msg pt(float x, float y)
        {
            common::FoxPoint2Msg p;
            p.x = x; p.y = y;
            return p;
        }
    }

} // namespace flychams::operator_pkg