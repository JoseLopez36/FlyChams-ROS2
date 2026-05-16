#pragma once

// Position solver
#include "flychams_common/positioning/position_solver.hpp"

// Solver algorithms
#include "flychams_common/assignment/suboptimal_combinatorial.hpp"

// Utilities
#include "flychams_common/types/core_types.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Solver for cluster-agent assignment problems
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-01-31
     * ════════════════════════════════════════════════════════════════
     */
    class AssignmentSolver
    {
    public: // Types
        using SharedPtr = std::shared_ptr<AssignmentSolver>;
        // Modes
        enum class SolverMode
        {
            SUBOPTIMAL_COMBINATORIAL
        };
        // Parameters
        struct Parameters
        {
            // Optimization weights
            float observation_weight;
            float distance_weight;
            float switch_weight;
        };

    private: // Parameters
        SolverMode mode_;

    private: // Data
        // Solver algorithms
        SuboptimalCombinatorial suboptimal_combinatorial_;

    public: // Public methods
        // Configuration
        void init(const SolverMode& mode, const Parameters& params)
        {
            // Set mode
            mode_ = mode;

            // Initialize the solver algorithms
            switch (mode_)
            {
                case SolverMode::SUBOPTIMAL_COMBINATORIAL:
                {
                    // Get Suboptimal Combinatorial parameters
                    SuboptimalCombinatorial::Parameters suboptimal_combinatorial_params;
                    suboptimal_combinatorial_params.observation_weight = params.observation_weight;
                    suboptimal_combinatorial_params.distance_weight = params.distance_weight;
                    suboptimal_combinatorial_params.switch_weight = params.switch_weight;

                    // Initialize the Suboptimal Combinatorial solver with the parameters
                    suboptimal_combinatorial_.init(suboptimal_combinatorial_params);
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid solver mode");
            }
        }

        void destroy()
        {
            // Destroy the solver algorithms
            switch (mode_)
            {
                case SolverMode::SUBOPTIMAL_COMBINATORIAL:
                {
                    suboptimal_combinatorial_.destroy();
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid solver mode");
            }
        }

        // Optimization
        common::RowVectorXi run(const common::Matrix3Xr& tab_x, const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r, const common::RowVectorXi& X_prev, const std::vector<common::Matrix4r>& wTcentral_array, std::vector<PositionSolver::SharedPtr>& solvers)
        {
            // Run the assignment based on the mode
            switch (mode_)
            {
                case SolverMode::SUBOPTIMAL_COMBINATORIAL:
                {
                    return suboptimal_combinatorial_.run(tab_x, tab_P, tab_r, X_prev, wTcentral_array, solvers);
                }

                default:
                    throw std::invalid_argument("Invalid solver mode");
            }
        }
    };

} // namespace flychams::common