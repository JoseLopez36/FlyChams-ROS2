#pragma once

// Standard includes
#include <cmath>
#include <vector>

// Core includes
#include "flychams_core/types/core_types.hpp"

namespace flychams::core
{
    class MathUtils
    {
    public:
        // ════════════════════════════════════════════════════════════════════════════
        // ANGLE: Angle utilities
        // ════════════════════════════════════════════════════════════════════════════

        /**
         * @brief Convert degrees to radians
         */
        static float degToRad(float degrees);

        /**
         * @brief Convert radians to degrees
         */
        static float radToDeg(float radians);

        /**
         * @brief Normalize angle to [-π, π]
         */
        static float normalizeAngle(float angle);

        // ════════════════════════════════════════════════════════════════════════════
        // VECTOR: Vector utilities
        // ════════════════════════════════════════════════════════════════════════════

        /**
         * @brief Compute the distance between two points
         */
        static float distance(const Vector3r& p1, const Vector3r& p2);

        /**
         * @brief Compute the direction between two points
         */
        static Vector3r direction(const Vector3r& from, const Vector3r& to);
    };

}  // namespace flychams::core