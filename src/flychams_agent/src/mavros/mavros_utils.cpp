#include "flychams_agent/mavros/mavros_utils.hpp"

using namespace flychams::common;

namespace flychams::agent
{
    Vector3r MavrosUtils::pointToNED(const Vector3r& enu)
    {
        Eigen::Vector3d enu_double = enu.cast<double>();
        Eigen::Vector3d ned_double = mavros::ftf::transform_frame_enu_ned(enu_double);
        return ned_double.cast<float>();
    }

    Vector3r MavrosUtils::pointFromNED(const Vector3r& ned)
    {
        Eigen::Vector3d ned_double = ned.cast<double>();
        Eigen::Vector3d enu_double = mavros::ftf::transform_frame_ned_enu(ned_double);
        return enu_double.cast<float>();
    }

    Quaternionr MavrosUtils::quatToNED(const Quaternionr& q)
    {
        return mavros::ftf::transform_orientation_enu_ned(q.cast<double>()).cast<float>();
    }

    Quaternionr MavrosUtils::quatFromNED(const Quaternionr& q)
    {
        return mavros::ftf::transform_orientation_ned_enu(q.cast<double>()).cast<float>();
    }

    Vector3r MavrosUtils::eulerToNED(const Vector3r& euler)
    {
        Quaternionr quat = MavrosUtils::eulerToQuat(euler);
        return MavrosUtils::quatToEuler(quatToNED(quat));
    }

    Vector3r MavrosUtils::eulerFromNED(const Vector3r& euler)
    {
        Quaternionr quat = MavrosUtils::eulerToQuat(euler);
        return MavrosUtils::quatToEuler(quatFromNED(quat));
    }

    GeoPointMsg MavrosUtils::toGlobal(const double& x, const double& y, const double& z, const GeoPointMsg& origin)
    {
        // Create LocalCartesian projector
        GeographicLib::LocalCartesian projector(origin.latitude, origin.longitude, origin.altitude, GeographicLib::Geocentric::WGS84());

        // Convert to global coordinates
        double lat, lon, alt;
        projector.Reverse(x, y, z, lat, lon, alt);

        // Create and return point
        GeoPointMsg point;
        point.latitude = lat;
        point.longitude = lon;
        point.altitude = alt;
        return point;
    }

    PointMsg MavrosUtils::fromGlobal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin)
    {
        // Create LocalCartesian projector
        GeographicLib::LocalCartesian projector(origin.latitude, origin.longitude, origin.altitude, GeographicLib::Geocentric::WGS84());

        // Convert to local coordinates
        double x, y, z;
        projector.Forward(lat, lon, alt, x, y, z);

        // Create and return point
        PointMsg point;
        point.x = x;
        point.y = y;
        point.z = z;
        return point;
    }

    Vector3r MavrosUtils::quatToEuler(const Quaternionr& q)
    {
        return mavros::ftf::quaternion_to_rpy(q.cast<double>()).cast<float>();
    }

    Quaternionr MavrosUtils::eulerToQuat(const Vector3r& euler)
    {
        return mavros::ftf::quaternion_from_rpy(euler.cast<double>()).cast<float>();
    }

} // namespace flychams::agent