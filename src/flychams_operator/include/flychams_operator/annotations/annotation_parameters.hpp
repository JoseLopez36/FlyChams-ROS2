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
        constexpr float kCentralRefMinSide  = 540.0f;  // 960×540
        constexpr float kTrackingRefMinSide = 270.0f;  // 480×270

        inline float fromView(int view_w, int view_h, float ref_min_side)
        {
            return static_cast<float>(std::min(view_w, view_h)) / ref_min_side;
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // HUD OVERLAY PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace HudAnnotations
    {
        constexpr bool kEnabled = true;

        // Header / footer strips (full view width)
        constexpr float kBarHeight       = 30.0f;
        constexpr float kBarAccentThick  = 1.5f;
        constexpr float kBarPadX         = 10.0f;
        constexpr float kBarFontSize     = 17.0f;
        constexpr float kBarRoleFontSize = 16.0f;
        constexpr float kTextWidthFactor = 0.58f;
        constexpr float kTextWidthNarrow = 0.78f;
        constexpr float kTextWidthDegree = 1.05f;   // ° and × render wider than ASCII
        constexpr float kTextWidthRightSlack = 0.25f;

        // Crosshair
        constexpr float kCrosshairArmFrac  = 0.045f;
        constexpr float kCrosshairGapFrac  = 0.013f;
        constexpr float kCrosshairThick    = 1.75f;

        // Centre reticle dot
        constexpr float kCentreDotDiam     = 6.0f;
        constexpr float kCentreDotOutline  = 1.0f;

        // Colors (accent is agent palette colour at draw time)
        inline const Color kBarFill    = withAlpha(Colors::kBlack, 0.62f);
        inline const Color kBarText = withAlpha(Colors::kWhite,  0.70f);
        constexpr float    kRoleTextAlpha = 1.0f;
        inline const Color kCross   = withAlpha(Colors::kWhite,  0.35f);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CAMERA ANNOTATION PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace CameraAnnotations
    {
        // Window crop overlay (central view only)
        constexpr bool  kShowWindowsOnCentral = true;
        constexpr float kWinOverlayBoxThick   = 2.0f;
        constexpr float kWinOverlayIdFontSz   = 13.0f;
        constexpr float kWinOverlayIdFontMarginX = -1.0f;
        constexpr float kWinOverlayIdFontMarginY = -5.0f;
        // Colors (agent color is used dynamically from AgentColors palette)
        inline const Color kBg      = withAlpha(Colors::kBlack,       0.70f);
        inline const Color kWin     = withAlpha(Colors::kCyan,        0.70f);
        inline const Color kWinOob  = withAlpha(Colors::kScarlettRed, 0.70f);
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

        inline void addFilledRect(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float x0, float y0, float x1, float y1, common::FoxColorMsg fill)
        {
            common::FoxPointsAnnotationMsg rect;
            rect.type = common::FoxPointsAnnotationMsg::LINE_LOOP;
            rect.timestamp = stamp;
            rect.points = {pt(x0, y0), pt(x1, y0), pt(x1, y1), pt(x0, y1)};
            rect.fill_color = fill;
            msg.points.push_back(rect);
        }
    }

} // namespace flychams::operator_pkg