#pragma once

// GeographicLib
#include <GeographicLib/LocalCartesian.hpp>
#include <GeographicLib/Geocentric.hpp>

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/math_utils.hpp"

// MavROS includes
#include <mavros/frame_tf.hpp>

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Transformation utilities based on mavROS frame_tf.hpp
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-07
     * ════════════════════════════════════════════════════════════════
     */
    class TfUtils
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
         * @brief Transform orientation from NED to ENU frame
         */
        static Quaternionr quatToNED(const Quaternionr& q);

        /**
         * @brief Transform orientation from ENU to NED frame
         */
        static Quaternionr quatFromNED(const Quaternionr& q);

        /**
         * @brief Transform euler angles from NED to ENU frame
         */
        static Vector3r eulerToNED(const Vector3r& euler);

        /**
         * @brief Transform euler angles from ENU to NED frame
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
    };
} // namespace flychams::core

