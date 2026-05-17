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
         * @brief Transform a point from ENU to NED frame
         */
        static Vector3r pointToNED(const Vector3r& enu);

        /**
         * @brief Transform a point from NED to ENU frame
         */
        static Vector3r pointFromNED(const Vector3r& ned);

        /**
         * @brief Transform orientation from ENU to NED frame
         */
        static Quaternionr quatToNED(const Quaternionr& q);

        /**
         * @brief Transform orientation from NED to ENU frame
         */
        static Quaternionr quatFromNED(const Quaternionr& q);

        /**
         * @brief Transform Euler angles (RPY) from ENU to NED frame
         */
        static Vector3r eulerToNED(const Vector3r& euler);

        /**
         * @brief Transform Euler angles (RPY) from NED to ENU frame
         */
        static Vector3r eulerFromNED(const Vector3r& euler);

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