#pragma once

// Tracking includes
#include "flychams_common/tracking/zoom_utils.hpp"

// Utilities
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/utils/math_utils.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Solver for agent tracking. Specific for camera tracking
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-04-28
     * ════════════════════════════════════════════════════════════════
     */
    class CameraSolver
    {
    public: // Types
        using SharedPtr = std::shared_ptr<CameraSolver>;
        // Modes
        enum class AimingMode
        {
            INITIAL,
            CONTINUOUS
        };

    private: // Data
        float focal_prev_;
        common::Vector3r rpy_prev_;
        bool is_first_update_;

    public: // Public methods
        // Configuration
        void reset()
        {
            // Reset tracking data
            focal_prev_ = 0.0f;
            rpy_prev_.setZero();
            is_first_update_ = true;
        }

        // Runtime methods
        std::tuple<float, float, common::Vector3r, float> run(const common::Vector3r& z, const float& r, const common::Matrix4r& T, const common::ObservationUnitParameters& unit_params)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // T: Ci_ in world frame (auxiliary frame)
            // unit_params: Observation unit parameters

            // Extract camera position and orientation
            const common::Vector3r x = T.block<3, 1>(0, 3);
            const common::Matrix3r R = T.block<3, 3>(0, 0);

            // Compute focal length
            const float focal = computeFocal(z, r, x, unit_params);

            // Compute upsilon (focal length for cameras)
            const float upsilon = focal;

            // Update previous focal length
            focal_prev_ = focal;

            // Compute camera orientation
            common::Vector3r rpy;
            if (is_first_update_)
            {
                rpy = computeCameraOrientation(z, x, R, common::Vector3r(), true);
                is_first_update_ = false;
            }
            else
            {
                rpy = computeCameraOrientation(z, x, R, rpy_prev_, false);
            }

            // Update previous orientation
            rpy_prev_ = rpy;

            // Compute apparent target size
            const float d = (x - z).norm();
            const float apparent_size = ZoomUtils::computeApparentSize(r, upsilon, d, unit_params.rho);

            // Return upsilon, focal length, orientation and apparent size
            return std::make_tuple(upsilon, focal, rpy, apparent_size);
        }

    private: // Implementation
        float computeFocal(const common::Vector3r& z, const float& r, const common::Vector3r& x, const common::ObservationUnitParameters& unit_params)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // x: Camera position in world frame (m)
            // unit_params: Observation unit parameters

            // Extract parameters
            const auto& f_min = unit_params.upsilon_min;
            const auto& f_max = unit_params.upsilon_max;
            const auto& s_ref = unit_params.s_ref;

            // Compute distance between target and camera
            float d = (x - z).norm();

            // Attempt to adjust the focal length to achieve the desired apparent size of the object
            float f = (d * s_ref) / r;

            // Clamp the focal length within the camera's focal limits
            f = std::max(std::min(f, f_max), f_min);

            // Return focal length
            return f;
        }

        common::Vector3r computeCameraOrientation(const common::Vector3r& z, const common::Vector3r& x, const common::Matrix3r& wRc, const common::Vector3r& prev_rpy, const bool& is_first_update)
        {
            // Args:
            // z: Target position in world frame (m)
            // x: Camera position in world frame (m)
            // R: Camera rotation matrix in world frame
            // prev_rpy: Previous orientation in RPY format (rad)
            // is_first_update: Whether it is the first update

            common::Vector3r rpy = common::Vector3r::Zero(); // roll, pitch, yaw

            // Determine aiming mode
            const AimingMode mode = is_first_update ? AimingMode::INITIAL : AimingMode::CONTINUOUS;

            // Compute direction vector
            const common::Vector3r t = z - x;
            const common::Vector3r v = t.normalized();

            // Handle vertical downward case
            if (std::abs(v.z() - 1.0f) < 1e-6f)
            {
                // In CONTINUOUS mode, take the previous yaw
                rpy(2) = (mode == AimingMode::CONTINUOUS) ? prev_rpy(2) : 0.0f;
                return rpy;
            }

            // Calculate base solutions
            const auto [yaw1, pitch1] = calculateCameraBaseSolution(v);
            const auto [yaw2, pitch2] = calculateCameraInvertedSolution(v);

            // Select solution based on mode
            // If it is the first iteration, we don't have a previous reference angle to establish
            // aiming mode CONTINUOUS (continuity in movement). We opt for mode INITIAL (non-inverted image).
            // Normalize target direction vector
            switch (mode)
            {
            case AimingMode::INITIAL:
                return common::Vector3r(0.0f, pitch1, yaw1);

            case AimingMode::CONTINUOUS:
            {
                return calculateCameraContinuousSolution(yaw1, pitch1, yaw2, pitch2, prev_rpy);
            }

            default:
                throw std::invalid_argument("Invalid aiming mode");
            }
        }

        std::pair<float, float> calculateCameraBaseSolution(const common::Vector3r& v)
        {
            return {
                std::atan2(v.x(), -v.y()),    // Yaw (Z-axis rotation)
                std::acos(v.z())              // Pitch (Y-axis rotation)
            };
        }

        std::pair<float, float> calculateCameraInvertedSolution(const common::Vector3r& v)
        {
            return {
                std::atan2(-v.x(), v.y()),    // Yaw (Z-axis rotation)
                -std::acos(v.z())             // Pitch (Y-axis rotation)
            };
        }

        common::Vector3r calculateCameraContinuousSolution(const float& yaw1, const float& pitch1, const float& yaw2, const float& pitch2, const common::Vector3r& prev_rpy)
        {
            // Normalize angles to [-pi, pi]
            const float prev_yaw = common::MathUtils::normalizeAngle(prev_rpy(2));
            const float norm_yaw1 = common::MathUtils::normalizeAngle(yaw1);
            const float norm_yaw2 = common::MathUtils::normalizeAngle(yaw2);

            // Calculate angular distances
            const float dist1 = std::abs(common::MathUtils::normalizeAngle(norm_yaw1 - prev_yaw));
            const float dist2 = std::abs(common::MathUtils::normalizeAngle(norm_yaw2 - prev_yaw));

            // Choose closest yaw solution
            return (dist1 <= dist2) ? common::Vector3r(0.0f, pitch1, norm_yaw1) : common::Vector3r(0.0f, pitch2, norm_yaw2);
        }
    };

} // namespace flychams::common
