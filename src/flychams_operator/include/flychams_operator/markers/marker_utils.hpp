#pragma once

#include "flychams_common/types/ros_types.hpp"

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // AGENT MARKER PARAMETERS
    // Tune these to adjust agent visibility
    // ════════════════════════════════════════════════════════════════════════════
    namespace AgentParameters
    {
        // Body sphere diameter (m)
        constexpr double kBodyDiamXY   = 4.0;
        constexpr double kBodyDiamZ    = 1.0;
        // Text label
        constexpr bool   kDisplayText      = true;
        constexpr float  kFontSize         = 1.5f;
        constexpr double kLabelZOffset     = 4.5;

        // Status colors ─────────────────────────────────────────────────────────
        // IDLE  : warm amber
        constexpr float kIdleBodyR = 1.00f, kIdleBodyG = 0.78f, kIdleBodyB = 0.00f, kIdleBodyA = 0.9f;
        // ACTIVE: vivid cyan
        constexpr float kActBodyR  = 0.00f, kActBodyG  = 0.85f, kActBodyB  = 1.00f, kActBodyA  = 0.9f;
        // ERROR : vivid red
        constexpr float kErrBodyR  = 1.00f, kErrBodyG  = 0.15f, kErrBodyB  = 0.10f, kErrBodyA  = 0.9f;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CLUSTER MARKER PARAMETERS
    // Tune these to adjust cluster visibility
    // ════════════════════════════════════════════════════════════════════════════
    namespace ClusterParameters
    {
        // Ring line thickness (m)
        constexpr float  kRingThickness  = 0.40f;
        // Bounding volume alpha
        constexpr float  kVolumeAlpha          = 0.08f;
        constexpr float  kRingAlpha            = 0.90f;
        // Text label
        constexpr bool   kDisplayText          = true;
        constexpr float  kFontSize             = 2.0f;
        constexpr double kLabelZExtraOffset    = 1.5;
        // Base color: vivid green
        constexpr float kR = 0.18f, kG = 1.00f, kB = 0.45f;
        constexpr int   kRingSegments          = 64;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // TARGET MARKER PARAMETERS
    // Tune these to adjust target visibility
    // ════════════════════════════════════════════════════════════════════════════
    namespace TargetParameters
    {
        // Body cylinder
        constexpr double kBodyDiamXY       = 1.2;
        constexpr double kBodyHeight       = 1.8;
        constexpr double kBodyZOffset      = 0.9;
        // Glow shell cylinder
        constexpr double kGlowDiamXY       = 3.5;
        constexpr double kGlowHeight       = 0.1;
        constexpr double kGlowZOffset      = 0.9;
        // Text label
        constexpr bool   kDisplayText      = false;
        constexpr float  kFontSize         = 2.0f;
        constexpr double kLabelZOffset     = 4.0;
        // Colors
        constexpr float kBodyR = 1.00f, kBodyG = 0.22f, kBodyB = 0.18f, kBodyA = 0.90f;
        constexpr float kGlowR = 1.00f, kGlowG = 0.30f, kGlowB = 0.10f, kGlowA = 0.20f;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // MARKER HELPERS
    // ════════════════════════════════════════════════════════════════════════════
    namespace MarkerHelpers
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