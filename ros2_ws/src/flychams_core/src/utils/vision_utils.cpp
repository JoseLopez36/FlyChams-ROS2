#include "flychams_core/utils/vision_utils.hpp"

namespace flychams::core
{
    float VisionUtils::computeFov(float focal, float sensor_width)
    {
        return 2.0f * std::atan((sensor_width / 2.0f) / focal);
    }

    Vector2r VisionUtils::projectPoint(const Vector3r& wP, const Matrix4r& wTc, const Matrix3r& K)
    {
        // Args:
        // - wP: World point
        // - wTc: World to camera transform
        // - K: Camera intrinsic matrix

        // Get transform from camera to world
        const Matrix4r cTw = wTc.inverse();

        // Get homogeneous point
        const Vector4r wP_ = wP.homogeneous();

        // Project point
        const Vector3r p = K * cTw.block<3, 4>(0, 0) * wP_;

        // Get projected coordinates
        const float u = p.x() / p.z();
        const float v = p.y() / p.z();

        // Return projected point
        return Vector2r(u, v);
    }

}  // namespace flychams::core