#pragma once

// Standard includes
#include <cmath>

// Utilities
#include "flychams_common/types/core_types.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Zoom factor (upsilon) utilities for observation units
     *
     * @details Camera: upsilon = focal length (m)
     *          Window: upsilon = lambda * xi
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-06-06
     * ════════════════════════════════════════════════════════════════
     */
    class ZoomUtils
    {
    public:
        /**
         * @brief Apparent target size (pix): s = r * upsilon / (d * rho)
         * @param r: Target radius (m)
         * @param upsilon: Zoom factor (upsilon)
         * @param d: Distance between target and camera (m)
         * @param rho: Regularized pixel size (m/pix)
         * @return Apparent target size (pix)
         */
        static float computeApparentSize(const float r, const float upsilon, const float d, const float rho)
        {
            return (r * upsilon) / (d * rho); // [pix]
        }

        /**
         * @brief Distance to target (m): d = (r * upsilon) / s
         * @param r: Target radius (m)
         * @param upsilon: Zoom factor (m)
         * @param s: Apparent target size (m)
         * @return Distance to target (m)
         */
        static float computeDistance(const float r, const float upsilon, const float s)
        {
            return (r * upsilon) / s; // [m]
        }

        /**
         * @brief Cluster offset from image centre (m)
         * @param p: Projected point on image (pix)
         * @param c: Image principal point (pix)
         * @param rho_x: Regularized pixel size in x direction (m/pix)
         * @param rho_y: Regularized pixel size in y direction (m/pix)
         * @return Cluster offset from image centre (m)
         */
        static float computeClusterOffset(const Vector2r& p, const Vector2r& c, const float rho_x, const float rho_y)
        {
            return std::sqrt(std::pow((p(0) - c(0)) * rho_x, 2) + std::pow((p(1) - c(1)) * rho_y, 2));
        }

        /**
         * @brief Offset correction factor: xi = sqrt(f^2 + l^2)
         * @param f: Central camera focal length (m)
         * @param l: Cluster offset from image centre (m)
         * @return Offset correction factor (m)
         */
        static float computeOffsetCorrectionFactor(const float f, const float l)
        {
            return std::sqrt(std::pow(f, 2) + std::pow(l, 2));
        }
    };

} // namespace flychams::common