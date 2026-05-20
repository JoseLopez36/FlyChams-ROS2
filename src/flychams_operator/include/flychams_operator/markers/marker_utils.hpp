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
        // Body sphere diameter (m) — primary UAV blob
        constexpr double kBodyDiamXY   = 3.0;
        constexpr double kBodyDiamZ    = 1.0;
        // Glow shell diameter (m) — translucent halo around body
        constexpr double kGlowDiamXY   = 6.0;
        constexpr double kGlowDiamZ    = 2.2;
        // Orientation arrow
        constexpr double kArrowShaftLen    = 4.5;
        constexpr double kArrowShaftDiam   = 0.30;
        constexpr double kArrowHeadLen     = 1.1;
        constexpr double kArrowHeadDiam    = 0.80;
        // Text label
        constexpr float  kFontSize         = 2.0f;     // world-units height
        constexpr double kLabelZOffset     = 4.5;      // above body centre

        // Status colors ─────────────────────────────────────────────────────────
        // IDLE  : warm amber
        constexpr float kIdleBodyR = 1.00f, kIdleBodyG = 0.78f, kIdleBodyB = 0.00f, kIdleBodyA = 1.0f;
        constexpr float kIdleGlowR = 1.00f, kIdleGlowG = 0.65f, kIdleGlowB = 0.00f, kIdleGlowA = 0.18f;
        // ACTIVE: vivid cyan
        constexpr float kActBodyR  = 0.00f, kActBodyG  = 0.85f, kActBodyB  = 1.00f, kActBodyA  = 1.0f;
        constexpr float kActGlowR  = 0.00f, kActGlowG  = 0.60f, kActGlowB  = 1.00f, kActGlowA  = 0.20f;
        // ERROR : vivid red
        constexpr float kErrBodyR  = 1.00f, kErrBodyG  = 0.15f, kErrBodyB  = 0.10f, kErrBodyA  = 1.0f;
        constexpr float kErrGlowR  = 1.00f, kErrGlowG  = 0.10f, kErrGlowB  = 0.00f, kErrGlowA  = 0.25f;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CLUSTER MARKER PARAMETERS
    // Tune these to adjust cluster visibility
    // ════════════════════════════════════════════════════════════════════════════
    namespace ClusterParameters
    {
        // Ring line thickness (m)
        constexpr float  kEquatorialThickness  = 0.40f;
        constexpr float  kMeridianThickness    = 0.20f;
        constexpr float  kRadiusThickness      = 0.25f;
        // Bounding volume alpha
        constexpr float  kVolumeAlpha          = 0.08f;
        constexpr float  kRingAlpha            = 0.90f;
        constexpr float  kMeridianAlpha        = 0.65f;
        constexpr float  kRadiusAlpha          = 0.65f;
        // Text label
        constexpr float  kFontSize             = 2.0f;
        constexpr double kLabelZExtraOffset    = 1.5;   // above top of sphere
        // Base color: vivid green
        constexpr float kR = 0.18f, kG = 1.00f, kB = 0.45f;
        constexpr int   kRingSegments          = 64;    // smoother rings at large radii
    }

    // ════════════════════════════════════════════════════════════════════════════
    // TARGET MARKER PARAMETERS
    // Tune these to adjust target visibility
    // ════════════════════════════════════════════════════════════════════════════
    namespace TargetParameters
    {
        // Body cylinder (human silhouette)
        constexpr double kBodyDiamXY       = 1.2;
        constexpr double kBodyHeight       = 1.8;
        constexpr double kBodyZOffset      = 0.9;   // lift above ground
        // Glow shell cylinder
        constexpr double kGlowDiamXY       = 3.0;
        constexpr double kGlowHeight       = 2.8;
        constexpr double kGlowZOffset      = 0.9;
        // Ground ring
        constexpr float  kRingThickness    = 0.20f;
        constexpr double kRingRadius       = 2.5;
        constexpr int    kRingSegments     = 48;
        // Text label
        constexpr float  kFontSize         = 2.0f;
        constexpr double kLabelZOffset     = 4.0;   // above top of glow
        // Colors
        constexpr float kBodyR = 1.00f, kBodyG = 0.22f, kBodyB = 0.18f, kBodyA = 1.0f;
        constexpr float kGlowR = 1.00f, kGlowG = 0.30f, kGlowB = 0.10f, kGlowA = 0.18f;
        constexpr float kRingR = 1.00f, kRingG = 0.55f, kRingB = 0.00f, kRingA = 0.85f;
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