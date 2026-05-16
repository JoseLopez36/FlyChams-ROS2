#pragma once

// GeographicLib
#include <GeographicLib/LocalCartesian.hpp>
#include <GeographicLib/Geocentric.hpp>

// Core includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/ros_types.hpp"
#include "flychams_common/utils/math_utils.hpp"

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
        static common::Vector3r pointToNED(const common::Vector3r& enu);

        /**
         * @brief Transform a point from NED to ENU frame
         */
        static common::Vector3r pointFromNED(const common::Vector3r& ned);

        /**
         * @brief Transform orientation from NED to ENU frame
         */
        static common::Quaternionr quatToNED(const common::Quaternionr& q);

        /**
         * @brief Transform orientation from ENU to NED frame
         */
        static common::Quaternionr quatFromNED(const common::Quaternionr& q);

        /**
         * @brief Transform euler angles from NED to ENU frame
         */
        static common::Vector3r eulerToNED(const common::Vector3r& euler);

        /**
         * @brief Transform euler angles from ENU to NED frame
         */
        static common::Vector3r eulerFromNED(const common::Vector3r& euler);

        /**
         * @brief Convert local cartesian coordinates (ENU) to global geographic coordinates (LLA)
         */
        static common::GeoPointMsg toGlobal(const double& x, const double& y, const double& z, const common::GeoPointMsg& origin);

        /**
         * @brief Convert global geographic coordinates (LLA) to local cartesian coordinates (ENU)
         */
        static common::PointMsg fromGlobal(const double& lat, const double& lon, const double& alt, const common::GeoPointMsg& origin);

        /**
         * @brief Transform orientation from quaternion to Euler angles (RPY)
         */
        static common::Vector3r quatToEuler(const common::Quaternionr& q);

        /**
         * @brief Transform orientation from Euler angles (RPY) to quaternion
         */
        static common::Quaternionr eulerToQuat(const common::Vector3r& euler);

        /**
         * @brief Convert a quaternion to a rotation matrix
         */
        static common::Matrix3r quatToMatrix(const common::Quaternionr& q);

        /**
         * @brief Convert a rotation matrix to a quaternion
         */
        static common::Quaternionr quatFromMatrix(const common::Matrix3r& matrix);
    };
} // namespace flychams::agent

