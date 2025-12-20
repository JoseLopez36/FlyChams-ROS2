#include "flychams_core/utils/tf_utils.hpp"

namespace flychams::core
{
    Vector3r TfUtils::pointToNED(const Vector3r& enu)
    {
        Eigen::Vector3d enu_double = enu.cast<double>();
        Eigen::Vector3d ned_double = mavros::ftf::transform_frame_enu_ned(enu_double);
        return ned_double.cast<float>();
    }

    Vector3r TfUtils::pointFromNED(const Vector3r& ned)
    {
        Eigen::Vector3d ned_double = ned.cast<double>();
        Eigen::Vector3d enu_double = mavros::ftf::transform_frame_ned_enu(ned_double);
        return enu_double.cast<float>();
    }

    Quaternionr TfUtils::quatToNED(const Quaternionr& q)
    {
        return mavros::ftf::transform_orientation_enu_ned(q.cast<double>()).cast<float>();
    }

    Quaternionr TfUtils::quatFromNED(const Quaternionr& q)
    {
        return mavros::ftf::transform_orientation_ned_enu(q.cast<double>()).cast<float>();
    }

    Vector3r TfUtils::eulerToNED(const Vector3r& euler)
    {
        Quaternionr quat = TfUtils::eulerToQuat(euler);
        return TfUtils::quatToEuler(quatToNED(quat));
    }

    Vector3r TfUtils::eulerFromNED(const Vector3r& euler)
    {
        Quaternionr quat = TfUtils::eulerToQuat(euler);
        return TfUtils::quatToEuler(quatFromNED(quat));
    }

    GeoPointMsg TfUtils::toGlobal(const double& x, const double& y, const double& z, const GeoPointMsg& origin)
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

    PointMsg TfUtils::fromGlobal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin)
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

    Vector3r TfUtils::quatToEuler(const Quaternionr& q)
    {
        return mavros::ftf::quaternion_to_rpy(q.cast<double>()).cast<float>();
    }

    Quaternionr TfUtils::eulerToQuat(const Vector3r& euler)
    {
        return mavros::ftf::quaternion_from_rpy(euler.cast<double>()).cast<float>();
    }

    Matrix3r TfUtils::quatToMatrix(const Quaternionr& q)
    {
        return q.toRotationMatrix();
    }

    Quaternionr TfUtils::quatFromMatrix(const Matrix3r& matrix)
    {
        return Quaternionr(matrix);
    }

}