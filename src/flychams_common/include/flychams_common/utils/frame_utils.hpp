#pragma once

// GeographicLib
#include <GeographicLib/LocalCartesian.hpp>
#include <GeographicLib/Geocentric.hpp>

// Core includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/ros_types.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Frame and coordinate transformation utilities
     * ════════════════════════════════════════════════════════════════
     * @details
     * Provides ENU⟺NED frame conversions, quaternion/Euler utilities,
     * and geodetic (LLA⟺ENU) conversions using GeographicLib.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-07
     * ════════════════════════════════════════════════════════════════
     */
    class FrameUtils
    {
    public:
        /**
         * @brief Convert local cartesian coordinates (ENU) to global geographic coordinates (LLA)
         */
        static GeoPointMsg toGlobal(const double& x, const double& y, const double& z, const GeoPointMsg& origin);

        /**
         * @brief Convert global geographic coordinates (LLA) to local cartesian coordinates (ENU)
         */
        static PointMsg fromGlobal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin);

        /**
         * @brief Transform orientation from quaternion to Euler angles (RPY)
         */
        static Vector3r quatToEuler(const Quaternionr& q);

        /**
         * @brief Transform orientation from Euler angles (RPY) to quaternion
         */
        static Quaternionr eulerToQuat(const Vector3r& euler);

        /**
         * @brief Convert a quaternion to a rotation matrix
         */
        static Matrix3r quatToMatrix(const Quaternionr& q);

        /**
         * @brief Convert a rotation matrix to a quaternion
         */
        static Quaternionr quatFromMatrix(const Matrix3r& matrix);
    };

} // namespace flychams::common