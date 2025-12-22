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

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Transformation utilities based on mavROS frame_tf.hpp
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-07
     * ════════════════════════════════════════════════════════════════
     */
    class MavrosUtils
    {
    public:
        /**
         * @brief Transform a point from ENU to NED frame
         */
        static core::Vector3r pointToNED(const core::Vector3r& enu);

        /**
         * @brief Transform a point from NED to ENU frame
         */
        static core::Vector3r pointFromNED(const core::Vector3r& ned);

        /**
         * @brief Transform orientation from NED to ENU frame
         */
        static core::Quaternionr quatToNED(const core::Quaternionr& q);

        /**
         * @brief Transform orientation from ENU to NED frame
         */
        static core::Quaternionr quatFromNED(const core::Quaternionr& q);

        /**
         * @brief Transform euler angles from NED to ENU frame
         */
        static core::Vector3r eulerToNED(const core::Vector3r& euler);

        /**
         * @brief Transform euler angles from ENU to NED frame
         */
        static core::Vector3r eulerFromNED(const core::Vector3r& euler);

        /**
         * @brief Convert local cartesian coordinates (ENU) to global geographic coordinates (LLA)
         */
        static core::GeoPointMsg toGlobal(const double& x, const double& y, const double& z, const core::GeoPointMsg& origin);

        /**
         * @brief Convert global geographic coordinates (LLA) to local cartesian coordinates (ENU)
         */
        static core::PointMsg fromGlobal(const double& lat, const double& lon, const double& alt, const core::GeoPointMsg& origin);

        /**
         * @brief Transform orientation from quaternion to Euler angles (RPY)
         */
        static core::Vector3r quatToEuler(const core::Quaternionr& q);

        /**
         * @brief Transform orientation from Euler angles (RPY) to quaternion
         */
        static core::Quaternionr eulerToQuat(const core::Vector3r& euler);

        /**
         * @brief Convert a quaternion to a rotation matrix
         */
        static core::Matrix3r quatToMatrix(const core::Quaternionr& q);

        /**
         * @brief Convert a rotation matrix to a quaternion
         */
        static core::Quaternionr quatFromMatrix(const core::Matrix3r& matrix);
    };
} // namespace flychams::agent

