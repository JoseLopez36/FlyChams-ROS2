#pragma once

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Centralised colour palette for operator visualisation
 *
 * @details
 * Defines the base color constants used across markers and
 * annotations. Provides two named color sets:
 * - AgentColors: per-agent palette (8 distinct colors, none red)
 *   used for agent body, clusters and annotations.
 * - TargetColors: fixed colors for target markers, chosen to
 *   contrast clearly with all agent palette entries.
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-05-23
 * ════════════════════════════════════════════════════════════════
 */

#include "flychams_common/types/ros_types.hpp"

namespace flychams::operator_pkg
{

    // ════════════════════════════════════════════════════════════════════════════
    // BASE COLORS
    // ════════════════════════════════════════════════════════════════════════════

    using Color = common::FoxColorMsg;

    // ── Factory helper ────────────────────────────────────────────────────────
    inline Color makeColor(float r, float g, float b, float a)
    {
        Color c;
        c.r = r; c.g = g; c.b = b; c.a = a;
        return c;
    }

    // ── Copy with new alpha ───────────────────────────────────────────────────
    inline Color withAlpha(const Color& c, float a)
    {
        return makeColor(c.r, c.g, c.b, a);
    }

    namespace Colors
    {
        // ── Neutrals ──────────────────────────────────────────────────────────
        inline const Color kBlack       = makeColor(0.00f, 0.00f, 0.00f, 1.00f);
        inline const Color kWhite       = makeColor(1.00f, 1.00f, 1.00f, 1.00f);
        inline const Color kGray        = makeColor(0.50f, 0.50f, 0.50f, 1.00f);

        // ── Blues / Cyans ─────────────────────────────────────────────────────
        inline const Color kCyan        = makeColor(0.00f, 0.85f, 1.00f, 1.00f);
        inline const Color kSkyBlue     = makeColor(0.00f, 0.55f, 1.00f, 1.00f);
        inline const Color kTeal        = makeColor(0.00f, 0.80f, 0.65f, 1.00f);

        // ── Yellows / Ambers ──────────────────────────────────────────────────
        inline const Color kAmber       = makeColor(1.00f, 0.78f, 0.00f, 1.00f);
        inline const Color kYellow      = makeColor(0.90f, 0.90f, 0.10f, 1.00f);
        inline const Color kPeach       = makeColor(1.00f, 0.65f, 0.40f, 1.00f);

        // ── Greens ────────────────────────────────────────────────────────────
        inline const Color kGreen       = makeColor(0.00f, 1.00f, 0.45f, 1.00f);
        inline const Color kLime        = makeColor(0.20f, 0.80f, 0.40f, 1.00f);

        // ── Reds / Oranges ────────────────────────────────────────────────────
        inline const Color kRed         = makeColor(1.00f, 0.10f, 0.10f, 1.00f);
        inline const Color kOrange      = makeColor(1.00f, 0.35f, 0.10f, 1.00f);
        inline const Color kScarlettRed = makeColor(1.00f, 0.20f, 0.00f, 1.00f);

        // ── Purples / Roses ───────────────────────────────────────────────────
        inline const Color kViolet      = makeColor(0.55f, 0.20f, 1.00f, 1.00f);
        inline const Color kRose        = makeColor(1.00f, 0.20f, 0.60f, 1.00f);
        inline const Color kMagenta     = makeColor(0.85f, 0.00f, 0.85f, 1.00f);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // AGENT COLORS
    // ════════════════════════════════════════════════════════════════════════════

    namespace AgentColors
    {
        constexpr int kPaletteSize = 8;
        inline const Color kPalette[kPaletteSize] = {
            Colors::kCyan,      // 0 — bright blue-cyan
            Colors::kViolet,    // 1 — purple
            Colors::kAmber,     // 2 — golden yellow
            Colors::kTeal,      // 3 — blue-green
            Colors::kPeach,     // 4 — warm orange
            Colors::kSkyBlue,   // 5 — medium blue
            Colors::kLime,      // 6 — lime green
            Colors::kMagenta,   // 7 — magenta
        };

        inline const Color& get(int agent_idx)
        {
            return kPalette[agent_idx % kPaletteSize];
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // TARGET COLORS
    // ════════════════════════════════════════════════════════════════════════════

    namespace TargetColors
    {
        inline const Color kBody  = withAlpha(Colors::kScarlettRed, 1.00f);
        inline const Color kLabel = withAlpha(Colors::kRed,         0.90f);
    }

} // namespace flychams::operator_pkg