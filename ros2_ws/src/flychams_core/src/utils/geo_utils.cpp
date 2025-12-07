#include "flychams_core/utils/geo_utils.hpp"

namespace flychams::core
{
    GeoPointMsg GeoUtils::toGlobal(const double& x, const double& y, const double& z, const GeoPointMsg& origin)
    {
        // Create LocalCartesian projector
        GeographicLib::LocalCartesian proj(origin.latitude, origin.longitude, origin.altitude, GeographicLib::Geocentric::WGS84());
        
        // Convert to global coordinates
        double lat, lon, alt;
        proj.Reverse(x, y, z, lat, lon, alt);

        // Create and return point
        GeoPointMsg point;
        point.latitude = lat;
        point.longitude = lon;
        point.altitude = alt;
        return point;
    }

    PointMsg GeoUtils::toLocal(const double& lat, const double& lon, const double& alt, const GeoPointMsg& origin)
    {
        // Create LocalCartesian projector
        GeographicLib::LocalCartesian proj(origin.latitude, origin.longitude, origin.altitude, GeographicLib::Geocentric::WGS84());
        
        // Convert to local coordinates
        double x, y, z;
        proj.Forward(lat, lon, alt, x, y, z);

        // Create and return point
        PointMsg point;
        point.x = x;
        point.y = y;
        point.z = z;
        return point;
    }
}

