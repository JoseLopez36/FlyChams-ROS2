#pragma once

// Tracking includes
#include "flychams_agent/tracking/camera_solver.hpp"
#include "flychams_agent/tracking/window_solver.hpp"

// Utilities
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/utils/math_utils.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Solver for observation unit tracking
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-09-04
     * ════════════════════════════════════════════════════════════════
     */
    class ObservationSolver
    {
    public: // Constructor
        ObservationSolver(const core::ObservationUnitParameters& unit_params)
            : unit_params_(unit_params)
        {
            // Create solvers based on unit type
            if (unit_params.type == core::ObservationType::Camera)
            {
                camera_solver_ = std::make_shared<CameraSolver>();
            }
            else if (unit_params.type == core::ObservationType::Window)
            {
                window_solver_ = std::make_shared<WindowSolver>();
            }
            else
            {
                throw std::invalid_argument("Invalid observation unit type");
            }
        }

    public: // Types
        using SharedPtr = std::shared_ptr<ObservationSolver>;

    private: // Parameters
        core::ObservationUnitParameters unit_params_;

    private: // Data
        // Solver used for Camera type
        CameraSolver::SharedPtr camera_solver_;
        // Solver used for Window type
        WindowSolver::SharedPtr window_solver_;

    public: // Public methods
        // Configuration
        void reset()
        {
            // Reset solvers based on unit type
            if (unit_params_.type == core::ObservationType::Camera)
            {
                camera_solver_->reset();
            }
            else if (unit_params_.type == core::ObservationType::Window)
            {
                window_solver_->reset();
            }
        }

        // Runtime methods
        std::tuple<float, core::Vector3r> runCamera(const core::Vector3r& z, const float& r, const core::Matrix4r& T)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // T: Ci_ in world frame (auxiliary frame)

            // Run camera solver
            return camera_solver_->run(z, r, T, unit_params_);
        }

        std::tuple<float, core::Crop> runWindow(const core::Vector3r& z, const float& r, const core::Matrix4r& T)
        {
            // Args:
            // z: Target position in world frame (m)
            // r: Equivalent radius of the target's area of interest (m)
            // T: C0_ in world frame (auxiliary frame of central camera)

            // Run window solver
            return window_solver_->run(z, r, T, unit_params_);
        }
    };

} // namespace flychams::agent