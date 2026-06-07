#pragma once

// Utilities
#include "flychams_common/tracking/zoom_utils.hpp"
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/utils/vision_utils.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Cost functions for agent positioning
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-04-17
     * ════════════════════════════════════════════════════════════════
     */
    class CostFunctions
    {
    public: // Types
        struct UnitCostParameters // Parameters for the cost of a single observation unit
        {
            // Unit parameters
            common::ObservationUnitParameters params;

            // Cost function weights for this unit
            // Psi
            float tau0 = 1.0f;
            float tau1 = 2.0f;
            float tau2 = 10.0f;
            // Lambda
            float sigma0 = 1.0f;
            float sigma1 = 2.0f;
            float sigma2 = 10.0f;
            // Gamma
            float mu = 1.0f;
            float nu = 1.0f;
        };
        struct CostParameters // Parameters for the cost function
        {
            int n_t;                                    // Number of tracking units
            std::vector<UnitCostParameters> units;      // Unit cost parameters
        };

    public: // Cost functions without gradient calculation
        static float J0(const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r, const common::Vector3r& x, const CostParameters& cost_params)
        {
            // Compute the value of the optimization index based on nested intervals 
            // (original cost function, with non-convex term)
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_t; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index
                J += CostFunctions::unitJ0(z, r, x, unit);
            }

            // Return the value of J
            return J;
        }

        static float J1(const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r, const common::Vector3r& x, const CostParameters& cost_params)
        {
            // Compute the value of the optimization index based on nested intervals 
            // (without non-convex term)
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_t; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index
                J += CostFunctions::unitJ1(z, r, x, unit);
            }

            // Return the value of J
            return J;
        }

        static float J2(const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r, const common::Vector3r& x, const common::Vector3r& x_hat, const CostParameters& cost_params)
        {
            // Compute the value of the optimization index based on nested intervals 
            // (with convex relaxation of the non-convex term)
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_t; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index
                J += CostFunctions::unitJ2(z, r, x, x_hat, unit);
            }

            // Return the value of J
            return J;
        }

    public: // Cost functions with gradient calculation
        static float J1(const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r, const common::Vector3r& x, const CostParameters& cost_params, common::Vector3r& grad)
        {
            // Initialize the gradient
            grad = common::Vector3r::Zero();

            // Compute the value of the optimization index based on nested intervals 
            // (without non-convex term)
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_t; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index
                common::Vector3r grad_i;
                J += CostFunctions::unitJ1(z, r, x, unit, grad_i);

                // Integrate the gradient
                grad += grad_i;
            }

            // Return the value of J
            return J;
        }

        static float J2(const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r, const common::Vector3r& x, const common::Vector3r& x_hat, const CostParameters& cost_params, common::Vector3r& grad)
        {
            // Initialize the gradient
            grad = common::Vector3r::Zero();

            // Compute the value of the optimization index based on nested intervals 
            // (with convex relaxation of the non-convex term)
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_t; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index
                common::Vector3r grad_i;
                J += CostFunctions::unitJ2(z, r, x, x_hat, unit, grad_i);

                // Integrate the gradient
                grad += grad_i;
            }

            // Return the value of J
            return J;
        }

    public: // Cost for single observation unit without gradient calculation
        static float unitJ0(const common::Vector3r& z, const float& r, const common::Vector3r& x, const UnitCostParameters& unit)
        {
            // Original cost function with non-convex term
            // Not valid for non-global optimization (e.g. Ellipsoid method, Nelder-Mead Simplex...)
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // unit: parameters for the cost function for the observation unit

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& upsilon_min = unit.params.upsilon_min;
            const auto& upsilon_max = unit.params.upsilon_max;
            const auto& upsilon_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& sigma0 = unit.sigma0;
            const auto& sigma1 = unit.sigma1;
            const auto& sigma2 = unit.sigma2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Calculate the reference distance to the target
            const float d_ref = ZoomUtils::computeDistance(r, upsilon_ref, s_ref);

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            common::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = ZoomUtils::computeDistance(r, upsilon_min, s_ref);
            const float U1 = ZoomUtils::computeDistance(r, upsilon_max, s_ref);
            const float L2 = ZoomUtils::computeDistance(r, upsilon_min, s_max);
            const float U2 = ZoomUtils::computeDistance(r, upsilon_max, s_min);

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            const float lambda_i =
                sigma0 * pow(max(0.0f, L0 - d), 2) +
                sigma1 * pow(max(0.0f, L1 - d), 2) +
                sigma2 * pow(max(0.0f, L2 - d), 2);
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * common::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

        static float unitJ1(const common::Vector3r& z, const float& r, const common::Vector3r& x, const UnitCostParameters& unit)
        {
            // Cost function without non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // unit: parameters for the cost function for the observation unit

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& upsilon_min = unit.params.upsilon_min;
            const auto& upsilon_max = unit.params.upsilon_max;
            const auto& upsilon_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Calculate the reference distance to the target
            const float d_ref = ZoomUtils::computeDistance(r, upsilon_ref, s_ref);

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            common::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = ZoomUtils::computeDistance(r, upsilon_min, s_ref);
            const float U1 = ZoomUtils::computeDistance(r, upsilon_max, s_ref);
            const float L2 = ZoomUtils::computeDistance(r, upsilon_min, s_max);
            const float U2 = ZoomUtils::computeDistance(r, upsilon_max, s_min);

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            // const float lambda_i = 0.0f; // Non-convex term is not considered
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * common::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + gamma_i;
        }

        static float unitJ2(const common::Vector3r& z, const float& r, const common::Vector3r& x, const common::Vector3r& x_hat, const UnitCostParameters& unit)
        {
            // Cost function with convex relaxation of the non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // x_hat: estimated position of the vehicle
            // unit: parameters for the cost function for the observation unit

            // Distance threshold to consider that x_hat coincides with zi
            // Considering that hMin is several meters, it should not be reached unless set very high, since zi are at height 0
            float eps_dist = 0.1f;

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& upsilon_min = unit.params.upsilon_min;
            const auto& upsilon_max = unit.params.upsilon_max;
            const auto& upsilon_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& sigma0 = unit.sigma0;
            const auto& sigma1 = unit.sigma1;
            const auto& sigma2 = unit.sigma2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const common::Vector3r v = x_hat - z;
            const float v_norm = v.norm();
            common::Vector3r eta = common::Vector3r::Zero();
            if (v_norm > eps_dist)
                eta = v / v_norm;

            // Calculate the projection as a substitute for distance for the non-convex term
            const float d_proj = (x - z).transpose() * eta;

            // Calculate the reference distance to the target
            const float d_ref = ZoomUtils::computeDistance(r, upsilon_ref, s_ref);

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            common::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = ZoomUtils::computeDistance(r, upsilon_min, s_ref);
            const float U1 = ZoomUtils::computeDistance(r, upsilon_max, s_ref);
            const float L2 = ZoomUtils::computeDistance(r, upsilon_min, s_max);
            const float U2 = ZoomUtils::computeDistance(r, upsilon_max, s_min);

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            const float lambda_i =
                sigma0 * pow(max(0.0f, L0 - d_proj), 2) +
                sigma1 * pow(max(0.0f, L1 - d_proj), 2) +
                sigma2 * pow(max(0.0f, L2 - d_proj), 2);
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * common::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

    public: // Cost for single observation unit with gradient calculation
        static float unitJ1(const common::Vector3r& z, const float& r, const common::Vector3r& x, const UnitCostParameters& unit, common::Vector3r& grad)
        {
            // Cost function without non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // unit: parameters for the cost function for the observation unit
            // grad: gradient of the cost function to be computed

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& upsilon_min = unit.params.upsilon_min;
            const auto& upsilon_max = unit.params.upsilon_max;
            const auto& upsilon_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Initialize the gradient
            grad = common::Vector3r::Zero();

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const common::Vector3r v_rho = x - z;
            const float v_rho_norm = v_rho.norm();
            common::Vector3r rho = common::Vector3r::Zero();
            if (v_rho_norm > 0.001f)
                rho = v_rho / v_rho_norm;

            // Calculate the reference distance to the target
            const float d_ref = ZoomUtils::computeDistance(r, upsilon_ref, s_ref);

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            common::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = ZoomUtils::computeDistance(r, upsilon_min, s_ref);
            const float U1 = ZoomUtils::computeDistance(r, upsilon_max, s_ref);
            const float L2 = ZoomUtils::computeDistance(r, upsilon_min, s_max);
            const float U2 = ZoomUtils::computeDistance(r, upsilon_max, s_min);

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            // const float lambda_i = 0.0f; // Non-convex term is not considered
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * common::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Compute the gradient of the cost function
            // Psi gradient
            float grad_psi = 2.0f *
                (tau0 * max(0.0f, d - U0) * heaviside(d, U0) +
                    tau1 * max(0.0f, d - U1) * heaviside(d, U1) +
                    tau2 * max(0.0f, d - U2) * heaviside(d, U2));
            // Gamma gradient
            common::Vector3r grad_gamma;
            grad_gamma(0) = 2.0f * (x(0) - p_ref(0)) + 2.0f * (d - (x(2) - z(2))) * (x(0) - z(0)) / d;
            grad_gamma(1) = 2.0f * (x(1) - p_ref(1)) + 2.0f * (d - (x(2) - z(2))) * (x(1) - z(1)) / d;
            grad_gamma(2) = 2.0f * (x(2) - p_ref(2)) + 2.0f * (d - (x(2) - z(2))) * ((x(2) - z(2)) / d - 1.0f);
            // Integrate all the gradients
            grad += grad_psi * rho + grad_gamma;

            // Return the value of Ji
            return psi_i + gamma_i;
        }

        static float unitJ2(const common::Vector3r& z, const float& r, const common::Vector3r& x, const common::Vector3r& x_hat, const UnitCostParameters& unit, common::Vector3r& grad)
        {
            // Cost function with convex relaxation of the non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // x_hat: estimated position of the vehicle
            // unit: parameters for the cost function for the observation unit
            // grad: gradient of the cost function to be computed

            // Distance threshold to consider that x_hat coincides with zi
            // Considering that hMin is several meters, it should not be reached unless set very high, since zi are at height 0
            float eps_dist = 0.1f;

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& upsilon_min = unit.params.upsilon_min;
            const auto& upsilon_max = unit.params.upsilon_max;
            const auto& upsilon_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& sigma0 = unit.sigma0;
            const auto& sigma1 = unit.sigma1;
            const auto& sigma2 = unit.sigma2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Initialize the gradient
            grad = common::Vector3r::Zero();

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const common::Vector3r v = x_hat - z;
            const float v_norm = v.norm();
            common::Vector3r eta = common::Vector3r::Zero();
            if (v_norm > eps_dist)
                eta = v / v_norm;

            const common::Vector3r v_rho = x - z;
            const float v_rho_norm = v_rho.norm();
            common::Vector3r rho = common::Vector3r::Zero();
            if (v_rho_norm > 0.001f)
                rho = v_rho / v_rho_norm;

            // Calculate the projection as a substitute for distance for the non-convex term
            const float d_proj = (x - z).transpose() * eta;

            // Calculate the reference distance to the target
            const float d_ref = ZoomUtils::computeDistance(r, upsilon_ref, s_ref);

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            common::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = ZoomUtils::computeDistance(r, upsilon_min, s_ref);
            const float U1 = ZoomUtils::computeDistance(r, upsilon_max, s_ref);
            const float L2 = ZoomUtils::computeDistance(r, upsilon_min, s_max);
            const float U2 = ZoomUtils::computeDistance(r, upsilon_max, s_min);

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            const float lambda_i =
                sigma0 * pow(max(0.0f, L0 - d_proj), 2) +
                sigma1 * pow(max(0.0f, L1 - d_proj), 2) +
                sigma2 * pow(max(0.0f, L2 - d_proj), 2);
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * common::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Compute the gradient of the cost function
            // Psi gradient
            float grad_psi = 2.0f *
                (tau0 * max(0.0f, d - U0) * heaviside(d, U0) +
                    tau1 * max(0.0f, d - U1) * heaviside(d, U1) +
                    tau2 * max(0.0f, d - U2) * heaviside(d, U2));
            // Lambda gradient
            float grad_lambda = -2.0f *
                (sigma0 * max(0.0f, L0 - d_proj) * heaviside(L0, d_proj) +
                    sigma1 * max(0.0f, L1 - d_proj) * heaviside(L1, d_proj) +
                    sigma2 * max(0.0f, L2 - d_proj) * heaviside(L2, d_proj));
            // Gamma gradient
            common::Vector3r grad_gamma;
            grad_gamma(0) = 2.0f * (x(0) - p_ref(0)) + 2.0f * (d - (x(2) - z(2))) * (x(0) - z(0)) / d;
            grad_gamma(1) = 2.0f * (x(1) - p_ref(1)) + 2.0f * (d - (x(2) - z(2))) * (x(1) - z(1)) / d;
            grad_gamma(2) = 2.0f * (x(2) - p_ref(2)) + 2.0f * (d - (x(2) - z(2))) * ((x(2) - z(2)) / d - 1.0f);
            // Integrate all the gradients
            grad += grad_psi * rho + grad_lambda * eta + grad_gamma;

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

    private: // Utils
        static float max(const float& a, const float& b)
        {
            return std::max(a, b);
        }

        static float heaviside(const float& d, const float& U)
        {
            if (d < U)
                return 0.0f;
            else if (d > U)
                return 1.0f;
            else
                return 0.5f;
        }
    };

} // namespace flychams::common