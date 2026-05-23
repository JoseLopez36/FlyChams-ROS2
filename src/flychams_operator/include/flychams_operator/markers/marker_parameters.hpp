#pragma once

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Centralised marker sizing, style and helper parameters
 *
 * @details
 * Contains constexpr namespaces for each element type (Agent,
 * Cluster, Target) and the MarkerHelpers utility for stamping
 * Foxglove SceneEntity messages.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-05-23
 * ════════════════════════════════════════════════════════════════
 */

#include "flychams_operator/colors/color_dictionary.hpp"

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // AGENT MARKER PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace AgentParameters
    {
        // Central body disc
        constexpr double kBodyDiamXY       = 0.45;
        constexpr double kBodyDiamZ        = 0.12;
        constexpr float  kBodyAlpha        = 0.90f;
        // Motor arms
        constexpr double kArmLength        = 0.70;
        constexpr double kArmDiam          = 0.05;
        // Rotor discs
        constexpr double kRotorDiam        = 0.55;
        constexpr double kRotorThickness   = 0.03;
        constexpr float  kRotorAlpha       = 0.50f;
        // Heading arrow
        constexpr bool   kShowArrow        = true;
        constexpr double kArrowShaftLen    = 0.50;
        constexpr double kArrowShaftDiam   = 0.04;
        constexpr double kArrowHeadLen     = 0.15;
        constexpr double kArrowHeadDiam    = 0.10;
        // Text label
        constexpr bool   kDisplayText      = true;
        constexpr float  kFontSize         = 1.5f;
        constexpr double kLabelXOffset     = -3.0;
        constexpr double kLabelYOffset     = 3.0;
        constexpr double kLabelZOffset     = 1.5;
        // Setpoint
        constexpr bool   kShowSetpoint          = true;
        constexpr double kSetpointDiam          = 0.30;
        constexpr double kSetpointLineThickness = 0.05;
        // Setpoint colors
        inline const Color kSetpoint     = withAlpha(Colors::kGreen, 0.90f);
        inline const Color kSetpointLine = withAlpha(Colors::kGreen, 0.50f);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CLUSTER MARKER PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace ClusterParameters
    {
        // Ring line thickness (m)
        constexpr float  kRingThickness        = 0.40f;
        constexpr int    kRingSegments         = 64;
        // Bounding volume alpha
        constexpr float  kVolumeAlpha          = 0.08f;
        constexpr float  kRingAlpha            = 0.90f;
        // Text label
        constexpr bool   kDisplayText          = true;
        constexpr float  kFontSize             = 2.0f;
        constexpr double kLabelZExtraOffset    = 1.5;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // TARGET MARKER PARAMETERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace TargetParameters
    {
        // Body cylinder
        constexpr double kBodyDiamXY       = 1.2;
        constexpr double kBodyHeight       = 1.8;
        constexpr double kBodyZOffset      = 0.9;
        // Text label
        constexpr bool   kDisplayText      = false;
        constexpr float  kFontSize         = 2.0f;
        constexpr double kLabelZOffset     = 4.0;
        // Colors
        inline const Color kBody = withAlpha(Colors::kScarlettRed, 1.00f);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // MARKER HELPERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace MarkerHelpers
    {
        inline void stampEntity(common::FoxSceneEntityMsg& entity, int64_t stamp_ns, const rclcpp::Duration& lifetime)
        {
            entity.timestamp.nanosec = static_cast<uint32_t>(stamp_ns % 1000000000LL);
            entity.timestamp.sec     = static_cast<int32_t>(stamp_ns  / 1000000000LL);
            entity.lifetime.sec      = static_cast<int32_t>(lifetime.seconds());
            entity.lifetime.nanosec  = static_cast<uint32_t>(lifetime.nanoseconds() % 1000000000LL);
            entity.frame_locked      = false;
        }
    }

} // namespace flychams::operator_pkg