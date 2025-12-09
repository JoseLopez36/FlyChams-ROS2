#include "flychams_core/utils/geo_utils.hpp"

namespace flychams::core
{
    GeoPointMsg GeoUtils::toGlobal(const double& x, const double& y, const double& z, const GeoPointMsg& origin)
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

    PointMsg GeoUtils::fromGlobal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin)
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

    Vector3r GeoUtils::toNED(const Vector3r& enu)
    {
        Eigen::Vector3d enu_double = enu.cast<double>();
        Eigen::Vector3d ned_double = mavros::ftf::transform_frame_enu_ned(enu_double);
        return ned_double.cast<float>();
    }

    Vector3r GeoUtils::fromNED(const Vector3r& ned)
    {
        Eigen::Vector3d ned_double = ned.cast<double>();
        Eigen::Vector3d enu_double = mavros::ftf::transform_frame_ned_enu(ned_double);
        return enu_double.cast<float>();
    }

    Quaternionr GeoUtils::toNED(const Quaternionr& q)
    {
        return mavros::ftf::transform_orientation_enu_ned(q.cast<double>()).cast<float>();
    }

    Quaternionr GeoUtils::fromNED(const Quaternionr& q)
    {
        return mavros::ftf::transform_orientation_ned_enu(q.cast<double>()).cast<float>();
    }
}