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
        // Central body disc
        constexpr double kBodyDiamXY       = 0.45;
        constexpr double kBodyDiamZ        = 0.12;
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
        // Status colors
        // IDLE
        constexpr float kIdleBodyR = 1.00f, kIdleBodyG = 0.78f, kIdleBodyB = 0.00f, kIdleBodyA = 0.9f;
        constexpr float kIdleRotorR = 1.00f, kIdleRotorG = 0.65f, kIdleRotorB = 0.00f;
        // ACTIVE
        constexpr float kActBodyR  = 0.00f, kActBodyG  = 0.85f, kActBodyB  = 1.00f, kActBodyA  = 0.9f;
        constexpr float kActRotorR = 0.00f, kActRotorG = 0.55f, kActRotorB = 1.00f;
        // ERROR
        constexpr float kErrBodyR  = 1.00f, kErrBodyG  = 0.15f, kErrBodyB  = 0.10f, kErrBodyA  = 0.9f;
        constexpr float kErrRotorR = 1.00f, kErrRotorG = 0.10f, kErrRotorB = 1.00f;
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
        // Base color
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
        // Text label
        constexpr bool   kDisplayText      = false;
        constexpr float  kFontSize         = 2.0f;
        constexpr double kLabelZOffset     = 4.0;
        // Colors
        constexpr float kBodyR = 1.00f, kBodyG = 0.22f, kBodyB = 0.18f, kBodyA = 1.0f;
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