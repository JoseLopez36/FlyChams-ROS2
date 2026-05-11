#pragma once

// Standard includes
#include <cmath>
#include <vector>

// OpenCV includes
#include <opencv2/opencv.hpp>

// Core includes
#include "flychams_common/types/core_types.hpp"

namespace flychams::core
{
    class VisionUtils
    {
    public:
        /**
         * @brief Compute the field of view of a camera
         */
        static float computeFov(float focal, float sensor_width);

        /**
         * @brief Project a 3D point onto a 2D image plane
         */
        static Vector2r projectPoint(const Vector3r& wP, const Matrix4r& wTc, const Matrix3r& K);
    };

}  // namespace flychams::core