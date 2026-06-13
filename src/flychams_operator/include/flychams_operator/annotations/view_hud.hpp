#pragma once

// Standard includes
#include <iomanip>
#include <sstream>

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Tactical HUD overlay for observation unit views
 *
 * @details
 * Renders crosshair and full-width header/footer strips for camera
 * and window annotation streams.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-06-13
 * ════════════════════════════════════════════════════════════════
 */

#include "flychams_common/types/core_types.hpp"
#include "flychams_operator/annotations/annotation_parameters.hpp"

namespace flychams::operator_pkg
{
    class ViewHud
    {
    public: // Types
        enum class Role : uint8_t
        {
            Central  = 1,
            Tracking = 2,
            Window   = 3,
        };

        struct Layout
        {
            int view_w = 0;
            int view_h = 0;
            float scale = 1.0f;
        };

        struct CameraSpec
        {
            Layout layout;
            common::ID agent_id;
            common::ID unit_id;
            Role role = Role::Central;
            float upsilon_norm = 1.0f;
            float pitch_deg = 0.0f;
            float yaw_deg = 0.0f;
            bool has_rotation = false;
            int agent_idx = 0;
        };

        struct WindowSpec
        {
            Layout layout;
            common::ID unit_id;
            float upsilon_norm = 1.0f;
            int crop_w = 0;
            int crop_h = 0;
            bool out_of_bounds = false;
            int agent_idx = 0;
        };

    public: // Drawing
        static void drawCamera(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const CameraSpec& spec);
        static void drawWindow(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const WindowSpec& spec);

    private: // Primitives
        static void drawCrosshair(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float cx, float cy, float side, float scale, const common::FoxColorMsg& accent, const common::FoxColorMsg& white);
        static void drawHeaderBar(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float scale, const common::FoxColorMsg& accent);
        static void drawFooterBar(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float H, float scale, const common::FoxColorMsg& accent);
        static void drawHeaderLabels(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float scale, const common::FoxColorMsg& accent, const std::string& left_text, const std::string& role_text, const std::string& right_text, bool role_warn = false);
        static void drawFooterCameraLabels(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float H, float scale, const common::FoxColorMsg& accent, float upsilon_norm, float pitch_deg, float yaw_deg, bool has_rotation);
        static void drawFooterWindowLabels(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float H, float scale, const common::FoxColorMsg& accent, float upsilon_norm, int crop_w, int crop_h);
        static void appendBarText(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float x, float baseline, const std::string& text, float font_sz, const common::FoxColorMsg& color, const common::FoxColorMsg& fill);
        static void appendBarTextRight(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, float W, float pad_x, float baseline, const std::string& text, float font_sz, const common::FoxColorMsg& color, const common::FoxColorMsg& fill);
        static float estimateTextWidth(const std::string& text, float font_sz);
        static std::string roleLabel(Role role);
    };

} // namespace flychams::operator_pkg
