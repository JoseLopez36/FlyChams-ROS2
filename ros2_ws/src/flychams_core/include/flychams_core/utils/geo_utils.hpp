#pragma once

// GeographicLib
#include <GeographicLib/LocalCartesian.hpp>
#include <GeographicLib/Geocentric.hpp>

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/math_utils.hpp"

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
        static PointMsg toLocal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin);
    };
}

