#pragma once

// Tracking includes
#include "flychams_common/tracking/zoom_utils.hpp"

// Utilities
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Solver for agent tracking. Specific for window tracking
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-04-28
     * ════════════════════════════════════════════════════════════════
     */
    class WindowSolver
    {
    public: // Types
        using SharedPtr = std::shared_ptr<WindowSolver>;

    public: // Public methods
        // Configuration
        void reset()
        {
            // Nothing to reset
        }

        // Runtime methods
        std::tuple<float, float, common::Crop, float> run(const common::Vector3r& z, const float& r, const common::Matrix4r& T, const float& f, const common::ObservationUnitParameters& unit_params)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // T: C0 in world frame (frame of central camera)
            // f: Central camera focal length (m)
            // unit_params: Observation unit parameters

            // Extract camera position
            const common::Vector3r x = T.block<3, 1>(0, 3);

            // Project target position onto central camera
            common::Matrix3r K = unit_params.camera_params.K;
            K(0, 0) = f / unit_params.rho_x;
            K(1, 1) = f / unit_params.rho_y;
            common::Vector2r p = common::VisionUtils::projectPoint(z, T, K);

            // Compute window size, upsilon, lambda and apparent size
            const auto [size, upsilon, lambda, apparent_size] = computeWindowSize(z, r, x, p, f, unit_params);

            // Compute window corner
            const common::Vector2i corner = computeWindowCorner(p, size);

            // Check if crop is out of bounds (i.e. if the crop is completely outside the image)
            bool is_out_of_bounds =
                (corner(0) + size(0) <= 0) ||                               // Completely to the left
                (corner(1) + size(1) <= 0) ||                               // Completely above
                (corner(0) >= unit_params.window_params.full_width) ||      // Completely to the right
                (corner(1) >= unit_params.window_params.full_height);       // Completely below

            // Make crop struct
            common::Crop crop;
            crop.x = corner.x();
            crop.y = corner.y();
            crop.w = size.x();
            crop.h = size.y();
            crop.is_out_of_bounds = is_out_of_bounds;

            // Return upsilon, lambda, crop and apparent size
            return std::make_tuple(upsilon, lambda, crop, apparent_size);
        }

    private: // Implementation
        std::tuple<common::Vector2i, float, float, float> computeWindowSize(const common::Vector3r& z, const float& r, const common::Vector3r& x, const common::Vector2r& p, const float& f, const common::ObservationUnitParameters& unit_params)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // x: Central camera position in world frame (m)
            // p: Projected point on central camera (pix)
            // f: Central camera focal length (m)
            // unit_params: Observation unit parameters

            // Extract parameters
            const auto& f_ref = unit_params.window_params.f_ref;
            const auto& full_width = unit_params.window_params.full_width;
            const auto& full_height = unit_params.window_params.full_height;
            const auto& tracking_width = unit_params.window_params.tracking_width;
            const auto& tracking_height = unit_params.window_params.tracking_height;
            const auto& lambda_min = unit_params.window_params.lambda_min;
            const auto& lambda_max = unit_params.window_params.lambda_max;
            const auto& K = unit_params.camera_params.K;
            const auto& rho_x = unit_params.rho_x;
            const auto& rho_y = unit_params.rho_y;
            const auto& rho = unit_params.rho;
            const auto& s_ref = unit_params.s_ref;

            // Compute distance between target and camera
            const float d = (x - z).norm();

            // Calculate the correction factor for uncentered targets
            const Vector2r c(K(0, 2), K(1, 2));
            const float l = ZoomUtils::computeClusterOffset(p, c, rho_x, rho_y);
            const float xi = ZoomUtils::computeOffsetCorrectionFactor(f, l);

            // Attempt to adjust the resolution factor to achieve the desired apparent size of the object
            float lambda = (d * s_ref) / (r * xi);

            // Clamp the resolution factor within limits
            lambda = std::max(std::min(lambda, lambda_max), lambda_min);

            // Compute upsilon (lambda*f for windows)
            const float upsilon = lambda * f_ref;

            // Compute offset corrected upsilon (lambda*xi)
            const float upsilon_xi = lambda * xi;

            // Compute apparent target size
            const float apparent_size = ZoomUtils::computeApparentSize(r, upsilon_xi, d, rho);

            // Compute window size using the resolution factor
            common::Vector2i size(0, 0);
            size(0) = static_cast<int>(std::round(static_cast<float>(tracking_width) / lambda));
            size(1) = static_cast<int>(std::round(static_cast<float>(tracking_height) / lambda));

            // Return window size, upsilon, lambda and apparent size
            return std::make_tuple(size, upsilon, lambda, apparent_size);
        }

        common::Vector2i computeWindowCorner(const common::Vector2r& p, const common::Vector2i& size)
        {
            // Args:
            // p: Projected point on central camera (pix)
            // size: Window size (pix)

            // Compute window corner to place the crop in the center of the image
            const float x = p(0) - static_cast<float>(size(0)) / 2.0f;
            const float y = p(1) - static_cast<float>(size(1)) / 2.0f;

            return {
                static_cast<int>(std::round(x)),
                static_cast<int>(std::round(y))
            };
        }
    };

} // namespace flychams::common
