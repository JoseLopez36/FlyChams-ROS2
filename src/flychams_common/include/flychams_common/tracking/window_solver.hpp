#pragma once

// Utilities
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"

namespace flychams::core
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
        std::tuple<float, core::Crop> run(const core::Vector3r& z, const float& r, const core::Matrix4r& T, const core::ObservationUnitParameters& unit_params)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // T: C0 in world frame (frame of central camera)
            // unit_params: Observation unit parameters

            // Extract camera position
            const core::Vector3r x = T.block<3, 1>(0, 3);

            // Project target position onto central camera
            core::Vector2r p = core::VisionUtils::projectPoint(z, T, unit_params.camera_params.K);

            // Compute window size
            const auto [size, lambda] = computeWindowSize(z, r, x, p, unit_params);

            // Compute window corner
            const core::Vector2i corner = computeWindowCorner(p, size);

            // Check if crop is out of bounds (i.e. if the crop is completely outside the image)
            bool is_out_of_bounds =
                (corner(0) + size(0) <= 0) ||                               // Completely to the left
                (corner(1) + size(1) <= 0) ||                               // Completely above
                (corner(0) >= unit_params.window_params.full_width) ||      // Completely to the right
                (corner(1) >= unit_params.window_params.full_height);       // Completely below

            // Make crop struct
            core::Crop crop;
            crop.x = corner.x();
            crop.y = corner.y();
            crop.w = size.x();
            crop.h = size.y();
            crop.is_out_of_bounds = is_out_of_bounds;

            // Return resolution factor and crop
            return std::make_tuple(lambda, crop);
        }

    private: // Implementation
        std::tuple<core::Vector2i, float> computeWindowSize(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Vector2r& p, const core::ObservationUnitParameters& unit_params)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // x: Central camera position in world frame (m)
            // p: Projected point on central camera (pix)
            // unit_params: Observation unit parameters

            // Extract parameters
            const auto& f = unit_params.window_params.f_ref;
            const auto& full_width = unit_params.window_params.full_width;
            const auto& full_height = unit_params.window_params.full_height;
            const auto& tracking_width = unit_params.window_params.tracking_width;
            const auto& tracking_height = unit_params.window_params.tracking_height;
            const auto& lambda_min = unit_params.upsilon_min;
            const auto& lambda_max = unit_params.upsilon_max;
            const auto& rho_x = unit_params.rho_x;
            const auto& rho_y = unit_params.rho_y;
            const auto& s_ref = unit_params.s_ref;

            // Compute distance between target and camera
            float d = (x - z).norm();

            // Calculate the correction factor for uncentered targets
            float u_pix = full_width / 2.0f;
            float v_pix = full_height / 2.0f;
            float l = std::sqrt(std::pow(p(0) - u_pix, 2) * std::pow(rho_x, 2) + std::pow(p(1) - v_pix, 2) * std::pow(rho_y, 2));
            float xi = std::sqrt(std::pow(f, 2) + std::pow(l, 2));

            // Attempt to adjust the resolution factor to achieve the desired apparent size of the object
            float lambda = (d * s_ref) / (r * xi);

            // Clamp the resolution factor within the camera's resolution limits
            lambda = std::max(std::min(lambda, lambda_max), lambda_min);

            // Compute window size using the resolution factor
            core::Vector2i size(0, 0);
            size(0) = static_cast<int>(std::round(static_cast<float>(tracking_width) / lambda));
            size(1) = static_cast<int>(std::round(static_cast<float>(tracking_height) / lambda));

            // Return window size and resolution factor
            return std::make_tuple(size, lambda);
        }

        core::Vector2i computeWindowCorner(const core::Vector2r& p, const core::Vector2i& size)
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

} // namespace flychams::core