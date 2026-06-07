#pragma once

// Standard includes
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <random>

// Eigen includes
#include <Eigen/Eigenvalues>

// Cost functions
#include "flychams_common/positioning/cost_functions.hpp"

// Utilities
#include "flychams_common/types/core_types.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Solver for agent positioning using CMA-ES algorithm
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-09
     * ════════════════════════════════════════════════════════════════
     */
    class CMAESAlgorithm
    {
    public: // Types
        // Parameters
        struct Parameters
        {
            // Space constraints
            common::Vector3r x_min;
            common::Vector3r x_max;

            // Generic solver parameters
            float tol = 1e-5f;
            int max_iter = 100;
        };
        // Data
        struct Data
        {
            // Cost function data
            common::Matrix3Xr tab_P;
            common::RowVectorXr tab_r;

            // Cost function parameters
            CostFunctions::CostParameters cost_params;
        };

    private: // Parameters
        Parameters params_;

    private: // Data
        // Data
        Data data_;

        // Random number generator
        std::mt19937 rng_;
        std::normal_distribution<float> normal_dist_;

    public: // Public methods
        void init(const Parameters& params, const CostFunctions::CostParameters& cost_params)
        {
            // Check parameters
            if ((params.x_min.array() > params.x_max.array()).any() || params.tol <= 0.0f || params.max_iter <= 0)
            {
                throw std::invalid_argument("Invalid parameters");
            }

            // Store parameters
            params_ = params;
            data_.cost_params = cost_params;

            // Initialize data
            data_.tab_P = common::Matrix3Xr::Zero(3, data_.cost_params.n_o);
            data_.tab_r = common::RowVectorXr::Zero(data_.cost_params.n_o);

            // Initialize random number generator
            rng_ = std::mt19937(std::random_device{}());
            normal_dist_ = std::normal_distribution<float>(0.0f, 1.0f);
        }
        void destroy()
        {
            // Nothing to destroy
        }
        common::Vector3r run(const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r, float& J)
        {
            // Update data struct
            data_.tab_P = tab_P;
            data_.tab_r = tab_r;
            
            // Compute the optimal position
            common::Vector3r x_opt;
            J = optimize(x_opt);

            // Return the optimal position and the cost function value
            return x_opt;
        }

    private: // Optimization methods
        float optimize(common::Vector3r& x_opt)
        {
            // Calculate lambda, mu and sigma
            int lambda = 4 + floor(3 * log(3));
            int mu = static_cast<int>(lambda / 2);
            float sigma = 8.0f;

            // Calculate weights
            common::VectorXr weights(mu);
            for (int i = 0; i < mu; i++)
            {
                weights(i) = log(mu + 0.5f) - log(i + 1.0f);
            }
            weights /= weights.sum();
            float mueff = pow(weights.sum(), 2) / weights.array().square().sum();

            // Calculate B, C and C
            common::Matrix3r B = common::Matrix3r::Identity();
            common::Vector3r D = common::Vector3r::Ones();
            common::Matrix3r C = B * D.cwiseProduct(D).asDiagonal() * B.transpose();
            common::Matrix3r invsqrtC = B * D.cwiseInverse().asDiagonal() * B.transpose();

            // Initialize x mean to the center of the search space
            common::Vector3r x_mean = params_.x_min + (params_.x_max - params_.x_min) / 2.0f;

            // Initialize chi N
            float chi_N = sqrt(3.0f) * (1.0f - 1.0f / (4.0f * 3.0f) + 1.0f / (21.0f * 3.0f * 3.0f));

            // Initialize variables
            common::Matrix3Xr arz(3, lambda);
            common::Matrix3Xr arx(3, lambda);
            common::VectorXr arfitness(lambda);
            common::Vector3r ps = common::Vector3r::Zero();
            common::Vector3r pc = common::Vector3r::Zero();
            float cc = (4.0f + mueff / 3.0f) / (3.0f + 4.0f + 2.0f * mueff / 3.0f);
            float cs = (mueff + 2.0f) / (3.0f + mueff + 5.0f);
            float c1 = 2.0f / (pow(3.0f + 1.3f, 2) + mueff);
            float cmu = std::min(1.0f - c1, static_cast<float>(2.0f * (mueff - 2.0f + 1.0f / mueff) / (pow(3.0f + 2.0f, 2) + 2.0f * mueff / 2.0f)));
            float damps = 1.0f + 2.0f * std::max(0.0f, static_cast<float>(sqrt((mueff - 1.0f) / (3.0f + 1.0f)) - 1.0f)) + cs;
            int eigeneval = 0;

            // Initialize cost function value
            float J = HUGE_VALF, J_prev = HUGE_VALF;

            // CMA-ES iterations
            for (int iter = 0; iter < params_.max_iter; iter++)
            {
                for (int k = 0; k < lambda; k++)
                {
                    // Generate random vector from normal distribution
                    arz.col(k) = randomNormalVector();

                    // Create new candidate
                    arx.col(k) = x_mean + sigma * B * (D.asDiagonal() * arz.col(k));

                    // Evaluate candidate
                    arfitness(k) = CostFunctions::J0(data_.tab_P, data_.tab_r, arx.col(k), data_.cost_params);

                    // Count evaluations
                    iter++;
                }

                // Sort indices by fitness
                std::vector<int> arindex(lambda);
                std::iota(arindex.begin(), arindex.end(), 0);
                std::sort(arindex.begin(), arindex.end(), [&](int a, int b) { return arfitness(a) < arfitness(b); });

                // Store old mean
                common::Vector3r x_old = x_mean;
                x_mean = arx(Eigen::all, arindex).leftCols(mu) * weights;

                ps = (1.0f - cs) * ps + sqrt(cs * (2.0f - cs) * mueff) * invsqrtC * (x_mean - x_old) / sigma;
                bool h_sig = (ps.norm() / sqrt(1.0f - pow(1.0f - cs, 2.0f * iter / lambda)) / chi_N) < (1.4f + 2.0f / (3.0f + 1.0f));
                pc = (1.0f - cc) * pc + (h_sig ? 1.0f : 0.0f) * sqrt(cc * (2.0f - cc) * mueff) * (x_mean - x_old) / sigma;

                common::Matrix3r artmp = (1.0f / sigma) * (arx(Eigen::all, arindex).leftCols(mu) - x_old.replicate(1, mu));
                C = (1.0f - c1 - cmu) * C +
                    c1 * (pc * pc.transpose() + (1.0f - static_cast<float>(h_sig)) * cc * (2.0f - cc) * C) +
                    cmu * artmp * weights.asDiagonal() * artmp.transpose();

                // Update step size
                sigma *= exp((cs / damps) * (ps.norm() / chi_N - 1.0f));

                if (iter - eigeneval > lambda / (c1 + cmu))
                {
                    eigeneval = iter;
                    C = (C + C.transpose()) / 2.0f;
                    Eigen::SelfAdjointEigenSolver<common::Matrix3r> eig(C);
                    B = eig.eigenvectors();
                    D = eig.eigenvalues().cwiseSqrt();
                    invsqrtC = B * D.cwiseInverse().asDiagonal() * B.transpose();
                }

                // Check convergence and other stopping criteria
                if (arfitness(arindex[0]) <= params_.tol || D.maxCoeff() > 1e7f * D.minCoeff())
                    break;

                // Update cost function value
                J_prev = J;
                J = arfitness(arindex[0]);

                // Update the best position
                x_opt = arx.col(arindex[0]);

                // Check if the cost function value has not changed
                if (abs(J - J_prev) <= params_.tol)
                {
                    break;
                }
            }

            // Return the global best cost and position
            return J;
        }

        common::Vector3r randomNormalVector()
        {
            // Generate a random vector with normal distribution (mean 0, std 1)
            common::Vector3r r;
            r(0) = normal_dist_(rng_);
            r(1) = normal_dist_(rng_);
            r(2) = normal_dist_(rng_);
            return r;
        }
    };

} // namespace flychams::common