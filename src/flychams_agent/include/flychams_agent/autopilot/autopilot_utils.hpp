#pragma once

#include <px4_ros_com/frame_transforms.h>

#include "flychams_common/types/core_types.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief PX4 autopilot frame transform utilities
     *
     * @details
     * Thin inline wrappers around px4_ros_com::frame_transforms that
     * accept and return the project's float-precision Eigen types
     * (Vector3r, Quaternionr). Casting to double is handled internally.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-21
     * ════════════════════════════════════════════════════════════════
     */
    namespace AutopilotUtils
    {
        namespace ft = px4_ros_com::frame_transforms;

        // ════════════════════════════════════════════════════════════
        // ENU ⟺ NED POINT TRANSFORMS
        // ════════════════════════════════════════════════════════════

        /**
         * @brief Transform a point from ENU to NED local frame
         */
        inline common::Vector3r pointToNED(const common::Vector3r& enu)
        {
            const Eigen::Vector3d v = enu.cast<double>();
            return ft::enu_to_ned_local_frame(v).cast<float>();
        }

        /**
         * @brief Transform a point from NED to ENU local frame
         */
        inline common::Vector3r pointFromNED(const common::Vector3r& ned)
        {
            const Eigen::Vector3d v = ned.cast<double>();
            return ft::ned_to_enu_local_frame(v).cast<float>();
        }

        // ════════════════════════════════════════════════════════════
        // ENU ⟺ NED ORIENTATION TRANSFORMS
        // ════════════════════════════════════════════════════════════

        /**
         * @brief Transform orientation from NED to ENU frame
         */
        inline common::Quaternionr quatFromNED(const common::Quaternionr& q)
        {
            const Eigen::Quaterniond qd = q.cast<double>();
            return ft::ned_to_enu_orientation(qd).cast<float>();
        }

        /**
         * @brief Transform orientation from ENU to NED frame
         */
        inline common::Quaternionr quatToNED(const common::Quaternionr& q)
        {
            const Eigen::Quaterniond qd = q.cast<double>();
            return ft::enu_to_ned_orientation(qd).cast<float>();
        }

        // ════════════════════════════════════════════════════════════
        // PX4 ⟺ ROS ORIENTATION TRANSFORMS
        // PX4 format: aircraft-to-NED  |  ROS format: baselink-to-ENU
        // ════════════════════════════════════════════════════════════

        /**
         * @brief Convert PX4 orientation (aircraft→NED) to ROS orientation (baselink→ENU)
         */
        inline common::Quaternionr px4ToRosOrientation(const common::Quaternionr& q)
        {
            const Eigen::Quaterniond qd = q.cast<double>();
            return ft::px4_to_ros_orientation(qd).cast<float>();
        }

        /**
         * @brief Convert ROS orientation (baselink→ENU) to PX4 orientation (aircraft→NED)
         */
        inline common::Quaternionr rosToPx4Orientation(const common::Quaternionr& q)
        {
            const Eigen::Quaterniond qd = q.cast<double>();
            return ft::ros_to_px4_orientation(qd).cast<float>();
        }

    }

} // namespace flychams::agent