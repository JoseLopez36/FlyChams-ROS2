#include "flychams_common/utils/frame_utils.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// ENU ⟺ NED POINT TRANSFORMS
// ENU: X=East, Y=North, Z=Up
// NED: X=North, Y=East, Z=Down
// Mapping: NED.x = ENU.y,  NED.y = ENU.x,  NED.z = -ENU.z
// ════════════════════════════════════════════════════════════════════════════

Vector3r FrameUtils::pointToNED(const Vector3r& enu)
{
    return Vector3r(enu.y(), enu.x(), -enu.z());
}

Vector3r FrameUtils::pointFromNED(const Vector3r& ned)
{
    return Vector3r(ned.y(), ned.x(), -ned.z());
}

// ════════════════════════════════════════════════════════════════════════════
// ENU ⟺ NED QUATERNION TRANSFORMS
// The fixed rotation from ENU to NED is: Rz(π) * Rx(π/2) applied via
// a constant quaternion q_ned_enu = [0, sin(π/2)*cos(π/2), sin(π/2)*cos(π/2), 0]
// ════════════════════════════════════════════════════════════════════════════

static const Eigen::Quaterniond kENUtoNED(
    Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()) *
    Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitX())
);

Quaternionr FrameUtils::quatToNED(const Quaternionr& q)
{
    Eigen::Quaterniond qd = q.cast<double>();
    Eigen::Quaterniond result = kENUtoNED * qd;
    return result.cast<float>();
}

Quaternionr FrameUtils::quatFromNED(const Quaternionr& q)
{
    Eigen::Quaterniond qd = q.cast<double>();
    Eigen::Quaterniond result = kENUtoNED.inverse() * qd;
    return result.cast<float>();
}

// ════════════════════════════════════════════════════════════════════════════
// ENU ⟺ NED EULER TRANSFORMS
// ════════════════════════════════════════════════════════════════════════════

Vector3r FrameUtils::eulerToNED(const Vector3r& euler)
{
    Quaternionr quat = FrameUtils::eulerToQuat(euler);
    return FrameUtils::quatToEuler(FrameUtils::quatToNED(quat));
}

Vector3r FrameUtils::eulerFromNED(const Vector3r& euler)
{
    Quaternionr quat = FrameUtils::eulerToQuat(euler);
    return FrameUtils::quatToEuler(FrameUtils::quatFromNED(quat));
}

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
    Eigen::Quaterniond qd = q.cast<double>().normalized();
    Eigen::Vector3d rpy = qd.toRotationMatrix().eulerAngles(0, 1, 2);
    return rpy.cast<float>();
}

Quaternionr FrameUtils::eulerToQuat(const Vector3r& euler)
{
    Eigen::Quaterniond qd =
        Eigen::AngleAxisd(static_cast<double>(euler.z()), Eigen::Vector3d::UnitZ()) *
        Eigen::AngleAxisd(static_cast<double>(euler.y()), Eigen::Vector3d::UnitY()) *
        Eigen::AngleAxisd(static_cast<double>(euler.x()), Eigen::Vector3d::UnitX());
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