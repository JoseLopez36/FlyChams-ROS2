#pragma once

#include "flychams_common/types/ros_types.hpp"

namespace flychams::operator_pkg
{

    // ════════════════════════════════════════════════════════════════════════════
    // BASE COLORS
    // ════════════════════════════════════════════════════════════════════════════

    using Color = common::FoxColorMsg;

    namespace Colors
    {
        // ── Neutrals ──────────────────────────────────────────────────────────
        constexpr Color kBlack       = { 0.00f, 0.00f, 0.00f, 1.00f };
        constexpr Color kWhite       = { 1.00f, 1.00f, 1.00f, 1.00f };

        // ── Blues / Cyans ─────────────────────────────────────────────────────
        constexpr Color kCyan        = { 0.00f, 0.85f, 1.00f, 1.00f };
        constexpr Color kSkyBlue     = { 0.00f, 0.55f, 1.00f, 1.00f };
        constexpr Color kTeal        = { 0.00f, 0.80f, 0.65f, 1.00f };

        // ── Yellows / Ambers ──────────────────────────────────────────────────
        constexpr Color kAmber       = { 1.00f, 0.78f, 0.00f, 1.00f };
        constexpr Color kYellow      = { 0.90f, 0.90f, 0.10f, 1.00f };
        constexpr Color kPeach       = { 1.00f, 0.65f, 0.40f, 1.00f };

        // ── Greens ────────────────────────────────────────────────────────────
        constexpr Color kGreen       = { 0.00f, 1.00f, 0.45f, 1.00f };
        constexpr Color kLime        = { 0.20f, 0.80f, 0.40f, 1.00f };

        // ── Reds / Oranges ────────────────────────────────────────────────────
        constexpr Color kRed         = { 1.00f, 0.10f, 0.10f, 1.00f };
        constexpr Color kOrange      = { 1.00f, 0.35f, 0.10f, 1.00f };
        constexpr Color kScarlettRed = { 1.00f, 0.20f, 0.00f, 1.00f };

        // ── Purples / Roses ───────────────────────────────────────────────────
        constexpr Color kViolet      = { 0.55f, 0.20f, 1.00f, 1.00f };
        constexpr Color kRose        = { 1.00f, 0.20f, 0.60f, 1.00f };
        constexpr Color kMagenta     = { 0.85f, 0.00f, 0.85f, 1.00f };
    }

    // ════════════════════════════════════════════════════════════════════════════
    // AGENT COLORS
    // ════════════════════════════════════════════════════════════════════════════

    namespace AgentColors
    {
        constexpr int kPaletteSize = 4;
        constexpr Color kPalette[kPaletteSize] = {
            Colors::kPeach,     // 0
            Colors::kViolet,    // 1
            Colors::kGreen,     // 2
            Colors::kLime,      // 3
        };

        inline constexpr const Color& get(int agent_idx)
        {
            return kPalette[agent_idx % kPaletteSize];
        }
    }

} // namespace flychams::operator_pkg