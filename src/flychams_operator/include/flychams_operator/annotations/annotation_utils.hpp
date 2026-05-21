#pragma once

#include "flychams_common/types/ros_types.hpp"

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // CAMERA ANNOTATION PARAMETERS
    // Tune these to adjust the central / tracking camera overlays
    // ════════════════════════════════════════════════════════════════════════════
    namespace CameraAnnotations
    {
        // Crosshair
        constexpr float kCrosshairArmFrac  = 0.04f;
        constexpr float kCrosshairGapFrac  = 0.012f;
        constexpr float kCrosshairThick    = 2.0f;
        // Centre dot
        constexpr float kCentreDotDiam     = 7.5f;
        // Role badge text (top-left corner)
        constexpr bool  kShowBadge         = true;
        constexpr float kBadgeFontSize     = 15.0f;
        constexpr float kBadgeMarginX      = 5.0f;
        constexpr float kBadgeMarginY      = 20.0f;
        // HUD text (bottom-left corner)
        constexpr bool  kShowHud           = true;
        constexpr float kHudFontSize       = 15.0f;
        constexpr float kHudMarginX        = 5.0f;
        constexpr float kHudMarginY        = 7.5f;
        // Window-overlay style (drawn on the central image)
        constexpr bool  kShowWindowsOnCentral = true;
        constexpr float kWinOverlayBoxThick   = 2.0f;
        constexpr float kWinOverlayIdFontSz   = 13.0f;
        constexpr float kWinOverlayIdFontMarginX = -1.0f;
        constexpr float kWinOverlayIdFontMarginY = -5.0f;
        // Colors
        // Central
        constexpr float kCentralR = 0.00f, kCentralG = 0.90f, kCentralB = 1.00f, kCentralA = 0.95f;
        // Tracking
        constexpr float kTrackR   = 1.00f, kTrackG   = 0.78f, kTrackB   = 0.00f, kTrackA   = 0.95f;
        // Semi-transparent text background
        constexpr float kBgR      = 0.00f, kBgG      = 0.00f, kBgB      = 0.00f, kBgA      = 0.50f;
        // In-bounds window overlay: cyan
        constexpr float kWinR = 0.00f, kWinG = 0.90f, kWinB = 1.00f, kWinA = 0.80f;
        // OOB window overlay: red
        constexpr float kWinOobR = 1.00f, kWinOobG = 0.20f, kWinOobB = 0.00f, kWinOobA = 0.80f;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // WINDOW ANNOTATION PARAMETERS
    // Tune these to adjust the crop / ROI window overlays
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
        // In-bounds
        constexpr float kTextR   = 0.00f, kTextG   = 0.90f, kTextB   = 1.00f, kTextA   = 0.95f;
        // OOB
        constexpr float kOobTxtR = 1.00f, kOobTxtG = 0.35f, kOobTxtB = 0.00f, kOobTxtA = 0.95f;
        // Semi-transparent text background
        constexpr float kBgR     = 0.00f, kBgG     = 0.00f, kBgB     = 0.00f, kBgA     = 0.50f;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // ANNOTATION HELPERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace AnnotationHelpers
    {
        inline common::FoxColorMsg makeColor(float r, float g, float b, float a)
        {
            common::FoxColorMsg c;
            c.r = r; c.g = g; c.b = b; c.a = a;
            return c;
        }

        inline common::FoxColorMsg white(float a = 0.95f)
        {
            return makeColor(1.0f, 1.0f, 1.0f, a);
        }

        inline void addLine(common::FoxImageAnnotationsMsg& msg,
                            const rclcpp::Time& stamp,
                            common::FoxPoint2Msg p1, common::FoxPoint2Msg p2,
                            common::FoxColorMsg color, float thickness)
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