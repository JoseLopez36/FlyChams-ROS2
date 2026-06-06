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
            return r * upsilon / (d * rho);
        }

        /**
         * @brief Reference distance (m): d_ref = r * upsilon / s_ref
         * @param r: Target radius (m)
         * @param upsilon: Zoom factor (upsilon)
         * @param s_ref: Reference apparent target size (pix)
         * @return Reference distance (m)
         */
        static float computeReferenceDistance(const float r, const float upsilon, const float s_ref)
        {
            return r * upsilon / s_ref;
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
         * @brief Max offset at image corner (half-diagonal, m)
         * @param width: Full image width (pix)
         * @param height: Full image height (pix)
         * @param rho_x: Regularized pixel size in x direction (m/pix)
         * @param rho_y: Regularized pixel size in y direction (m/pix)
         * @return Max offset at image corner (m)
         */
        static float computeMaxClusterOffset(const int width, const int height, const Vector2r& c, const float rho_x, const float rho_y)
        {
            const float w = static_cast<float>(width);
            const float h = static_cast<float>(height);

            // Maximum offset from principal point to any image corner
            const float l00 = computeClusterOffset(Vector2r(0.0f, 0.0f), c, rho_x, rho_y);
            const float l10 = computeClusterOffset(Vector2r(w, 0.0f), c, rho_x, rho_y);
            const float l01 = computeClusterOffset(Vector2r(0.0f, h), c, rho_x, rho_y);
            const float l11 = computeClusterOffset(Vector2r(w, h), c, rho_x, rho_y);

            return std::max(std::max(l00, l10), std::max(l01, l11));
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

        /**
         * @brief Window upsilon bounds from config lambda limits and image geometry
         * @param width: Full image width (pix)
         * @param height: Full image height (pix)
         * @param c: Image principal point (pix)
         * @param rho_x: Regularized pixel size in x direction (m/pix)
         * @param rho_y: Regularized pixel size in y direction (m/pix)
         * @param lambda_min: Minimum lambda value
         * @param lambda_max: Maximum lambda value
         * @param lambda_ref: Reference lambda value
         * @param f: Central camera focal length (m)
         * @return Tuple containing minimum upsilon, maximum upsilon and reference upsilon
         */
        static std::tuple<float, float, float> computeWindowUpsilonBounds(const int width, const int height, const Vector2r& c, 
            const float rho_x, const float rho_y,
            const float lambda_min, const float lambda_max, const float lambda_ref, const float f)
        {
            // Compute maximum cluster offset
            const float l_max = computeMaxClusterOffset(width, height, c, rho_x, rho_y);

            // Compute minimum offset correction factor
            const float xi_min = f;
            const float xi_max = computeOffsetCorrectionFactor(f, l_max);
            const float xi_ref = xi_min; // Reference offset correction factor is at minimum offset correction factor

            // Compute window upsilon bounds
            const float upsilon_min = lambda_min * xi_min;
            const float upsilon_max = lambda_max * xi_max;
            const float upsilon_ref = lambda_ref * xi_ref;

            // Return upsilon bounds
            return std::make_tuple(upsilon_min, upsilon_max, upsilon_ref);
        }
    };

} // namespace flychams::common