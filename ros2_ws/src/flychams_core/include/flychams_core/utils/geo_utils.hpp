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
     * @brief Geographic utilities for coordinate transformations
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-07
     * ════════════════════════════════════════════════════════════════
     */
    class GeoUtils
    {
    public:
        /**
         * @brief Convert local cartesian coordinates (ENU) to global geographic coordinates (LLA)
         * @param x Local X in meters (East)
         * @param y Local Y in meters (North)
         * @param z Local Z in meters (Up)
         * @param origin Global origin (LLA)
         * @return Global geographic coordinates (LLA)
         */
        static GeoPointMsg toGlobal(const double& x, const double& y, const double& z, const GeoPointMsg& origin);

        /**
         * @brief Convert global geographic coordinates (LLA) to local cartesian coordinates (ENU)
         * @param lat Latitude in degrees
         * @param lon Longitude in degrees
         * @param alt Altitude in meters
         * @param origin Global origin (LLA)
         * @return Local cartesian coordinates (ENU)
         */
        static PointMsg fromGlobal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin);

        /**
         * @brief Transform a point from ENU to NED frame
         * @param x Local X in ENU
         * @param y Local Y in ENU
         * @param z Local Z in ENU
         * @return Point in NED
         */
        static Vector3r toNED(const Vector3r& enu);

        /**
         * @brief Transform a point from NED to ENU frame
         * @param x Local X in NED
         * @param y Local Y in NED
         * @param z Local Z in NED
         * @return Point in ENU
         */
        static Vector3r fromNED(const Vector3r& ned);

        /**
         * @brief Transform orientation from NED to ENU frame
         * @param q Orientation in NED
         * @return Orientation in ENU
         */
        static Quaternionr toNED(const Quaternionr& q);

        /**
         * @brief Transform orientation from ENU to NED frame
         * @param q Orientation in ENU
         * @return Orientation in NED
         */
        static Quaternionr fromNED(const Quaternionr& q);
    };
}

