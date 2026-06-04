#pragma once

// Position solver
#include "flychams_common/positioning/position_solver.hpp"

// Solver algorithms
#include "flychams_common/assignment/exhaustive_search.hpp"
#include "flychams_common/assignment/branch_and_bound.hpp"

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
            EXHAUSTIVE_SEARCH      = 0,
            BRANCH_AND_BOUND       = 1
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
        ExhaustiveSearch exhaustive_search_;
        BranchAndBound branch_and_bound_;

    public: // Public methods
        // Configuration
        void init(const SolverMode& mode, const Parameters& params)
        {
            // Set mode
            mode_ = mode;

            // Initialize the solver algorithms
            switch (mode_)
            {
                case SolverMode::EXHAUSTIVE_SEARCH:
                {
                    // Get Exhaustive Search parameters
                    ExhaustiveSearch::Parameters exhaustive_search_params;
                    exhaustive_search_params.observation_weight = params.observation_weight;
                    exhaustive_search_params.distance_weight = params.distance_weight;
                    exhaustive_search_params.switch_weight = params.switch_weight;

                    // Initialize the Exhaustive Search solver with the parameters
                    exhaustive_search_.init(exhaustive_search_params);
                    break;
                }

                case SolverMode::BRANCH_AND_BOUND:
                {
                    // Get Branch and Bound parameters
                    BranchAndBound::Parameters branch_and_bound_params;
                    branch_and_bound_params.observation_weight = params.observation_weight;
                    branch_and_bound_params.distance_weight = params.distance_weight;
                    branch_and_bound_params.switch_weight = params.switch_weight;

                    // Initialize the Branch and Bound solver with the parameters
                    branch_and_bound_.init(branch_and_bound_params);
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
                case SolverMode::EXHAUSTIVE_SEARCH:
                {
                    exhaustive_search_.destroy();
                    break;
                }

                case SolverMode::BRANCH_AND_BOUND:
                {
                    branch_and_bound_.destroy();
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
                case SolverMode::EXHAUSTIVE_SEARCH:
                {
                    return exhaustive_search_.run(tab_x, tab_P, tab_r, X_prev, wTcentral_array, solvers);
                }

                case SolverMode::BRANCH_AND_BOUND:
                {
                    return branch_and_bound_.run(tab_x, tab_P, tab_r, X_prev, wTcentral_array, solvers);
                }

                default:
                    throw std::invalid_argument("Invalid solver mode");
            }
        }
    };

} // namespace flychams::common