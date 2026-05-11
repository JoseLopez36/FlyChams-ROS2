#include "flychams_core/utils/math_utils.hpp"

namespace flychams::core
{
    // ════════════════════════════════════════════════════════════════════════════
    // ANGLE: Angle utilities
    // ════════════════════════════════════════════════════════════════════════════

    float MathUtils::degToRad(float degrees)
    {
        return degrees * M_PI / 180.0f;
    }

    float MathUtils::radToDeg(float radians)
    {
        return radians * 180.0f / M_PI;
    }

    float MathUtils::normalizeAngle(float angle)
    {
        // Normalize angle to [-π, π]
        return std::atan2(std::sin(angle), std::cos(angle));
    }

    // ════════════════════════════════════════════════════════════════════════════
    // VECTOR: Vector utilities
    // ════════════════════════════════════════════════════════════════════════════

    float MathUtils::distance(const Vector3r& p1, const Vector3r& p2)
    {
        return (p1 - p2).norm();
    }

    Vector3r MathUtils::direction(const Vector3r& from, const Vector3r& to)
    {
        Vector3r dir = to - from;
        if (dir.norm() < 1e-6) {
            return Vector3r::Zero();
        }
        return dir.normalized();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // GEOMETRY: Geometric utilities
    // ════════════════════════════════════════════════════════════════════════════

    Matrix3r MathUtils::quatToMatrix(const Quaternionr& q)
    {
        return q.toRotationMatrix();
    }

    Quaternionr MathUtils::quatFromMatrix(const Matrix3r& matrix)
    {
        return Quaternionr(matrix);
    }

}  // namespace flychams::core