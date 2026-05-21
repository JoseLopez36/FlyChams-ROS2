#include "flychams_common/utils/frame_utils.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// GEODETIC CONVERSIONS (LLA ⟺ ENU) via GeographicLib
// ════════════════════════════════════════════════════════════════════════════

GeoPointMsg FrameUtils::toGlobal(const double& x, const double& y, const double& z, const GeoPointMsg& origin)
{
    GeographicLib::LocalCartesian projector(origin.latitude, origin.longitude, origin.altitude, GeographicLib::Geocentric::WGS84());

    double lat, lon, alt;
    projector.Reverse(x, y, z, lat, lon, alt);

    GeoPointMsg point;
    point.latitude = lat;
    point.longitude = lon;
    point.altitude = alt;
    return point;
}

PointMsg FrameUtils::fromGlobal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin)
{
    GeographicLib::LocalCartesian projector(origin.latitude, origin.longitude, origin.altitude, GeographicLib::Geocentric::WGS84());

    double x, y, z;
    projector.Forward(lat, lon, alt, x, y, z);

    PointMsg point;
    point.x = x;
    point.y = y;
    point.z = z;
    return point;
}

// ════════════════════════════════════════════════════════════════════════════
// QUATERNION ⟺ EULER (RPY) via Eigen
// ════════════════════════════════════════════════════════════════════════════

Vector3r FrameUtils::quatToEuler(const Quaternionr& q)
{
    const Eigen::Quaterniond qd = q.cast<double>().normalized();
    const Eigen::Vector3d rpy = qd.toRotationMatrix().eulerAngles(0, 1, 2);
    return rpy.cast<float>();
}

Quaternionr FrameUtils::eulerToQuat(const Vector3r& euler)
{
    const Eigen::Vector3d v = euler.cast<double>();
    const Eigen::Quaterniond qd =
        Eigen::AngleAxisd(v.z(), Eigen::Vector3d::UnitZ()) *
        Eigen::AngleAxisd(v.y(), Eigen::Vector3d::UnitY()) *
        Eigen::AngleAxisd(v.x(), Eigen::Vector3d::UnitX());
    return qd.cast<float>();
}

// ════════════════════════════════════════════════════════════════════════════
// QUATERNION ⟺ ROTATION MATRIX via Eigen
// ════════════════════════════════════════════════════════════════════════════

Matrix3r FrameUtils::quatToMatrix(const Quaternionr& q)
{
    return q.toRotationMatrix();
}

Quaternionr FrameUtils::quatFromMatrix(const Matrix3r& matrix)
{
    return Quaternionr(matrix);
}