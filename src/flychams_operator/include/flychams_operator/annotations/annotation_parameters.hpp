#pragma once

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Centralised annotation style parameters and helpers
 *
 * @details
 * Contains constexpr namespaces for camera and window annotation
 * styles and the AnnotationHelpers utility for building common
 * Foxglove ImageAnnotation primitives.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-05-23
 * ════════════════════════════════════════════════════════════════
 */

#include "flychams_common/types/ros_types.hpp"
#include "flychams_operator/colors/color_dictionary.hpp"

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // RESOLUTION SCALING
    // ════════════════════════════════════════════════════════════════════════════
    namespace AnnotationScale
    {
        // Min-side of the view sizes the constexpr style values below were tuned for
        constexpr float kCentralRefMinSide  = 2160.0f;  // 3840×2160
        constexpr float kTrackingRefMinSide = 270.0f;  // 480×270

        inline float fromView(int view_w, int view_h, float ref_min_side)
        {
            return static_cast<float>(std::min(view_w, view_h)) / ref_min_side;
        }
    }

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
        // Colors (agent color is used dynamically from AgentColors palette)
        inline const Color kBg      = withAlpha(Colors::kBlack,       0.50f);
        inline const Color kWin     = withAlpha(Colors::kCyan,        0.50f);
        inline const Color kWinOob  = withAlpha(Colors::kScarlettRed, 0.50f);
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
        inline const Color kBg      = withAlpha(Colors::kBlack,  0.50f);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CLUSTER ANNOTATION PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace ClusterAnnotations
    {
        // Rim sampling
        constexpr int   kRimPoints    = 64;
        // Dashed polyline style
        constexpr int   kNDashes      = 18;
        constexpr float kDashFrac     = 0.50f;
        constexpr float kThickness    = 2.0f;
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

        inline void addDashedPolyline(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const std::vector<common::FoxPoint2Msg>& pts, common::FoxColorMsg color, float thickness, int n_dashes = 20, float dash_frac = 0.55f)
        {
            const int n = static_cast<int>(pts.size());
            if (n < 2)
                return;
            const float pts_per_seg   = static_cast<float>(n) / static_cast<float>(n_dashes);
            const int   dash_pts      = std::max(2, static_cast<int>(std::round(pts_per_seg * dash_frac)));
            const int   seg_pts       = std::max(dash_pts + 1, static_cast<int>(std::round(pts_per_seg)));
            int i = 0;
            while (i < n)
            {
                // Collect dash_pts consecutive points (wrapping)
                common::FoxPointsAnnotationMsg dash;
                dash.type          = common::FoxPointsAnnotationMsg::LINE_STRIP;
                dash.timestamp     = stamp;
                dash.outline_color = color;
                dash.thickness     = thickness;
                for (int j = 0; j < dash_pts; ++j)
                {
                    dash.points.push_back(pts[(i + j) % n]);
                }
                msg.points.push_back(dash);
                // Advance by full segment (dash + gap)
                i += seg_pts;
            }
        }

        inline common::FoxPoint2Msg pt(float x, float y)
        {
            common::FoxPoint2Msg p;
            p.x = x; p.y = y;
            return p;
        }

        inline common::FoxColorMsg makeColor(const Color& c) { return c; }
    }

} // namespace flychams::operator_pkg