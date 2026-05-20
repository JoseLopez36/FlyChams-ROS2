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
        constexpr float kCrosshairThick    = 1.5f;
        // Centre dot
        constexpr float kCentreDotDiam     = 5.0f;
        // Image corner brackets
        constexpr float kBracketFrac       = 0.06f;
        constexpr float kBracketThick      = 1.5f;
        // HUD text
        constexpr float kHudFontSize       = 11.0f;
        constexpr float kHudMarginX        = 6.0f;
        constexpr float kHudMarginY        = 18.0f;
        // Role badge text
        constexpr float kBadgeFontSize     = 9.5f;
        constexpr float kBadgeMarginX      = 5.0f;
        constexpr float kBadgeMarginY      = 14.0f;
        // Colors
        // Central
        constexpr float kCentralR = 0.00f, kCentralG = 0.90f, kCentralB = 1.00f, kCentralA = 0.95f;
        // Tracking
        constexpr float kTrackR   = 1.00f, kTrackG   = 0.78f, kTrackB   = 0.00f, kTrackA   = 0.95f;
        // White for brackets / dot outline
        constexpr float kWhiteR   = 1.00f, kWhiteG   = 1.00f, kWhiteB   = 1.00f, kWhiteA   = 0.70f;
        // Semi-transparent text background
        constexpr float kBgR      = 0.00f, kBgG      = 0.00f, kBgB      = 0.00f, kBgA      = 0.50f;
        // Flags
        constexpr bool  kShowBrackets   = true;
        constexpr bool  kShowBadge      = true;
        constexpr bool  kShowHud        = true;
        constexpr bool  kShowZoomBar    = true;
        // Zoom bar (drawn above HUD text)
        constexpr float kZoomBarW       = 80.0f;
        constexpr float kZoomBarH       = 4.0f;
        constexpr float kZoomBarMarginX = 6.0f;
        constexpr float kZoomBarMarginY = 28.0f;
        constexpr float kZoomMax        = 4.0f;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // WINDOW ANNOTATION PARAMETERS
    // Tune these to adjust the crop / ROI window overlays
    // ════════════════════════════════════════════════════════════════════════════
    namespace WindowAnnotations
    {
        // Bounding box
        constexpr float kBoxThick          = 2.0f;
        // Corner ticks
        constexpr float kTickFrac          = 0.15f;
        constexpr float kTickThick         = 2.5f;
        // Text label
        constexpr float kIdFontSize        = 10.5f;
        constexpr float kIdOffsetY         = 13.0f;
        // Info text
        constexpr float kInfoFontSize      = 10.0f;
        constexpr float kInfoOffsetY       = 4.0f;
        // Zoom bar
        constexpr bool  kShowZoomBar       = true;
        constexpr float kZoomBarH          = 3.5f;
        constexpr float kZoomBarMarginY    = 16.0f;
        constexpr float kZoomMax           = 4.0f;
        // Colors
        constexpr float kBoxR    = 1.00f, kBoxG    = 0.85f, kBoxB    = 0.00f, kBoxA    = 0.90f;
        constexpr float kTickR   = 1.00f, kTickG   = 1.00f, kTickB   = 1.00f, kTickA   = 0.85f;
        constexpr float kTextR   = 1.00f, kTextG   = 0.85f, kTextB   = 0.00f, kTextA   = 0.95f;
        // Colors
        constexpr float kOobBoxR = 1.00f, kOobBoxG = 0.25f, kOobBoxB = 0.00f, kOobBoxA = 0.90f;
        constexpr float kOobTxtR = 1.00f, kOobTxtG = 0.35f, kOobTxtB = 0.00f, kOobTxtA = 0.95f;
        // Zoom bar fill color
        constexpr float kZoomBarR = 0.10f, kZoomBarG = 0.80f, kZoomBarB = 0.30f, kZoomBarA = 0.75f;
        constexpr float kZoomBgR  = 0.20f, kZoomBgG  = 0.20f, kZoomBgB  = 0.20f, kZoomBgA  = 0.50f;
        // Semi-transparent text background
        constexpr float kBgR      = 0.00f, kBgG      = 0.00f, kBgB      = 0.00f, kBgA      = 0.45f;
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