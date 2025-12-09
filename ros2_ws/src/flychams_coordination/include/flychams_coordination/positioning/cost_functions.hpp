#pragma once

// Utilities
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/utils/math_utils.hpp"
#include "flychams_core/utils/vision_utils.hpp"

namespace flychams::coordination
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
            core::ObservationUnitParameters params;

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
            int n_o;                                    // Number of units
            std::vector<UnitCostParameters> units;      // Unit cost parameters
        };

    public: // Cost functions without gradient calculation
        static float J0(const core::Matrix3Xr& tab_P, const core::RowVectorXr& tab_r, const core::Vector3r& x, const core::Matrix4r& wTcentral, const CostParameters& cost_params)
        {
            // Compute the value of the optimization index based on nested intervals 
            // (original cost function, with non-convex term) based on unit type
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_o; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index based on unit type
                switch (unit.params.type)
                {
                case core::ObservationType::Camera:
                    J += CostFunctions::cameraJ0(z, r, x, unit);
                    break;

                case core::ObservationType::Window:
                    J += CostFunctions::windowJ0(z, r, x, wTcentral, unit);
                    break;

                default:
                    throw std::invalid_argument("Invalid observation unit type");
                }
            }

            // Return the value of J
            return J;
        }

        static float J1(const core::Matrix3Xr& tab_P, const core::RowVectorXr& tab_r, const core::Vector3r& x, const core::Matrix4r& wTcentral, const CostParameters& cost_params)
        {
            // Compute the value of the optimization index based on nested intervals 
            // (without non-convex term) based on unit type
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_o; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index based on unit type
                switch (unit.params.type)
                {
                case core::ObservationType::Camera:
                    J += CostFunctions::cameraJ1(z, r, x, unit);
                    break;

                case core::ObservationType::Window:
                    J += CostFunctions::windowJ1(z, r, x, wTcentral, unit);
                    break;

                default:
                    throw std::invalid_argument("Invalid observation unit type");
                }
            }

            // Return the value of J
            return J;
        }

        static float J2(const core::Matrix3Xr& tab_P, const core::RowVectorXr& tab_r, const core::Vector3r& x, const core::Vector3r& x_hat, const core::Matrix4r& wTcentral, const CostParameters& cost_params)
        {
            // Compute the value of the optimization index based on nested intervals 
            // (with convex relaxation of the non-convex term) based on unit type
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_o; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index based on unit type
                switch (unit.params.type)
                {
                case core::ObservationType::Camera:
                    J += CostFunctions::cameraJ2(z, r, x, x_hat, unit);
                    break;

                case core::ObservationType::Window:
                    J += CostFunctions::windowJ2(z, r, x, x_hat, wTcentral, unit);
                    break;

                default:
                    throw std::invalid_argument("Invalid observation unit type");
                }
            }

            // Return the value of J
            return J;
        }

    public: // Cost functions with gradient calculation
        static float J1(const core::Matrix3Xr& tab_P, const core::RowVectorXr& tab_r, const core::Vector3r& x, const core::Matrix4r& wTcentral, const CostParameters& cost_params, core::Vector3r& grad)
        {
            // Initialize the gradient
            grad = core::Vector3r::Zero();

            // Compute the value of the optimization index based on nested intervals 
            // (without non-convex term) based on unit type
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_o; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index based on unit type
                core::Vector3r grad_i;
                switch (unit.params.type)
                {
                case core::ObservationType::Camera:
                    J += CostFunctions::cameraJ1(z, r, x, unit, grad_i);
                    break;

                case core::ObservationType::Window:
                    J += CostFunctions::windowJ1(z, r, x, wTcentral, unit, grad_i);
                    break;

                default:
                    throw std::invalid_argument("Invalid observation unit type");
                }

                // Integrate the gradient
                grad += grad_i;
            }

            // Return the value of J
            return J;
        }

        static float J2(const core::Matrix3Xr& tab_P, const core::RowVectorXr& tab_r, const core::Vector3r& x, const core::Vector3r& x_hat, const core::Matrix4r& wTcentral, const CostParameters& cost_params, core::Vector3r& grad)
        {
            // Initialize the gradient
            grad = core::Vector3r::Zero();

            // Compute the value of the optimization index based on nested intervals 
            // (with convex relaxation of the non-convex term) based on unit type
            float J = 0.0f;
            for (int i = 0; i < cost_params.n_o; i++)
            {
                // Get relevant data
                const auto& z = tab_P.col(i);
                const auto& r = tab_r(i);
                const auto& unit = cost_params.units[i];

                // Compute the value of the index based on unit type
                core::Vector3r grad_i;
                switch (unit.params.type)
                {
                case core::ObservationType::Camera:
                    J += CostFunctions::cameraJ2(z, r, x, x_hat, unit, grad_i);
                    break;

                case core::ObservationType::Window:
                    J += CostFunctions::windowJ2(z, r, x, x_hat, wTcentral, unit, grad_i);
                    break;

                default:
                    throw std::invalid_argument("Invalid observation unit type");
                }

                // Integrate the gradient
                grad += grad_i;
            }

            // Return the value of J
            return J;
        }

    public: // Cost for single observation unit without gradient calculation
        static float cameraJ0(const core::Vector3r& z, const float& r, const core::Vector3r& x, const UnitCostParameters& unit)
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
            const auto& f_min = unit.params.upsilon_min;
            const auto& f_max = unit.params.upsilon_max;
            const auto& f_ref = unit.params.upsilon_ref;
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
            const float d_ref = r * f_ref / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = r * f_min / s_ref;
            const float U1 = r * f_max / s_ref;
            const float L2 = r * f_min / s_max;
            const float U2 = r * f_max / s_min;

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
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

        static float cameraJ1(const core::Vector3r& z, const float& r, const core::Vector3r& x, const UnitCostParameters& unit)
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
            const auto& f_min = unit.params.upsilon_min;
            const auto& f_max = unit.params.upsilon_max;
            const auto& f_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Calculate the reference distance to the target
            const float d_ref = r * f_ref / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = r * f_min / s_ref;
            const float U1 = r * f_max / s_ref;
            const float L2 = r * f_min / s_max;
            const float U2 = r * f_max / s_min;

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            // const float lambda_i = 0.0f; // Non-convex term is not considered
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + gamma_i;
        }

        static float cameraJ2(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Vector3r& x_hat, const UnitCostParameters& unit)
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
            const auto& f_min = unit.params.upsilon_min;
            const auto& f_max = unit.params.upsilon_max;
            const auto& f_ref = unit.params.upsilon_ref;
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
            const core::Vector3r v = x_hat - z;
            const float v_norm = v.norm();
            core::Vector3r eta = core::Vector3r::Zero();
            if (v_norm > eps_dist)
                eta = v / v_norm;

            // Calculate the projection as a substitute for distance for the non-convex term
            const float d_proj = (x - z).transpose() * eta;

            // Calculate the reference distance to the target
            const float d_ref = r * f_ref / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = r * f_min / s_ref;
            const float U1 = r * f_max / s_ref;
            const float L2 = r * f_min / s_max;
            const float U2 = r * f_max / s_min;

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
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

        static float windowJ0(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Matrix4r& T, const UnitCostParameters& unit)
        {
            // Original cost function with non-convex term
            // Not valid for non-global optimization (e.g. Ellipsoid method, Nelder-Mead Simplex...)
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // T: C0 in world frame (frame of central camera)
            // unit: parameters for the cost function for the observation unit

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& lambda_min = unit.params.upsilon_min;
            const auto& lambda_max = unit.params.upsilon_max;
            const auto& lambda_ref = unit.params.upsilon_ref;
            const auto& f = unit.params.window_params.f_ref;
            const auto& full_width = unit.params.window_params.full_width;
            const auto& full_height = unit.params.window_params.full_height;
            const auto& rho_x = unit.params.rho_x;
            const auto& rho_y = unit.params.rho_y;
            const auto& K = unit.params.camera_params.K;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& sigma0 = unit.sigma0;
            const auto& sigma1 = unit.sigma1;
            const auto& sigma2 = unit.sigma2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Project target position onto central camera
            core::Vector2r p = core::VisionUtils::projectPoint(z, T, K);

            // Calculate the correction factor for uncentered targets
            float u_pix = full_width / 2.0f;
            float v_pix = full_height / 2.0f;
            float l = std::sqrt(std::pow(p(0) - u_pix, 2) * std::pow(rho_x, 2) + std::pow(p(1) - v_pix, 2) * std::pow(rho_y, 2));
            float xi = std::sqrt(std::pow(f, 2) + std::pow(l, 2));

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Calculate the reference distance to the target
            const float d_ref = (r * lambda_ref * xi) / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = (r * lambda_min * xi) / s_ref;
            const float U1 = (r * lambda_max * xi) / s_ref;
            const float L2 = (r * lambda_min * xi) / s_max;
            const float U2 = (r * lambda_max * xi) / s_min;

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
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

        static float windowJ1(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Matrix4r& T, const UnitCostParameters& unit)
        {
            // Cost function without non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // T: C0 in world frame (frame of central camera)
            // unit: parameters for the cost function for the observation unit

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& lambda_min = unit.params.upsilon_min;
            const auto& lambda_max = unit.params.upsilon_max;
            const auto& lambda_ref = unit.params.upsilon_ref;
            const auto& f = unit.params.window_params.f_ref;
            const auto& full_width = unit.params.window_params.full_width;
            const auto& full_height = unit.params.window_params.full_height;
            const auto& rho_x = unit.params.rho_x;
            const auto& rho_y = unit.params.rho_y;
            const auto& K = unit.params.camera_params.K;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Project target position onto central camera
            core::Vector2r p = core::VisionUtils::projectPoint(z, T, K);

            // Calculate the correction factor for uncentered targets
            float u_pix = full_width / 2.0f;
            float v_pix = full_height / 2.0f;
            float l = std::sqrt(std::pow(p(0) - u_pix, 2) * std::pow(rho_x, 2) + std::pow(p(1) - v_pix, 2) * std::pow(rho_y, 2));
            float xi = std::sqrt(std::pow(f, 2) + std::pow(l, 2));

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Calculate the reference distance to the target
            const float d_ref = (r * lambda_ref * xi) / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = (r * lambda_min * xi) / s_ref;
            const float U1 = (r * lambda_max * xi) / s_ref;
            const float L2 = (r * lambda_min * xi) / s_max;
            const float U2 = (r * lambda_max * xi) / s_min;

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            // const float lambda_i = 0.0f; // Non-convex term is not considered
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + gamma_i;
        }

        static float windowJ2(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Vector3r& x_hat, const core::Matrix4r& T, const UnitCostParameters& unit)
        {
            // Cost function with convex relaxation of the non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // x_hat: estimated position of the vehicle
            // T: C0 in world frame (frame of central camera)
            // unit: parameters for the cost function for the observation unit

            // Distance threshold to consider that x_hat coincides with zi
            // Considering that hMin is several meters, it should not be reached unless set very high, since zi are at height 0
            float eps_dist = 0.1f;

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& lambda_min = unit.params.upsilon_min;
            const auto& lambda_max = unit.params.upsilon_max;
            const auto& lambda_ref = unit.params.upsilon_ref;
            const auto& f = unit.params.window_params.f_ref;
            const auto& full_width = unit.params.window_params.full_width;
            const auto& full_height = unit.params.window_params.full_height;
            const auto& rho_x = unit.params.rho_x;
            const auto& rho_y = unit.params.rho_y;
            const auto& K = unit.params.camera_params.K;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& sigma0 = unit.sigma0;
            const auto& sigma1 = unit.sigma1;
            const auto& sigma2 = unit.sigma2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Project target position onto central camera
            core::Vector2r p = core::VisionUtils::projectPoint(z, T, K);

            // Calculate the correction factor for uncentered targets
            float u_pix = full_width / 2.0f;
            float v_pix = full_height / 2.0f;
            float l = std::sqrt(std::pow(p(0) - u_pix, 2) * std::pow(rho_x, 2) + std::pow(p(1) - v_pix, 2) * std::pow(rho_y, 2));
            float xi = std::sqrt(std::pow(f, 2) + std::pow(l, 2));

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const core::Vector3r v = x_hat - z;
            const float v_norm = v.norm();
            core::Vector3r eta = core::Vector3r::Zero();
            if (v_norm > eps_dist)
                eta = v / v_norm;

            // Calculate the projection as a substitute for distance for the non-convex term
            const float d_proj = (x - z).transpose() * eta;

            // Calculate the reference distance to the target
            const float d_ref = (r * lambda_ref * xi) / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = (r * lambda_min * xi) / s_ref;
            const float U1 = (r * lambda_max * xi) / s_ref;
            const float L2 = (r * lambda_min * xi) / s_max;
            const float U2 = (r * lambda_max * xi) / s_min;

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
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

    public: // Cost for single tracking unit with gradient calculation
        static float cameraJ1(const core::Vector3r& z, const float& r, const core::Vector3r& x, const UnitCostParameters& unit, core::Vector3r& grad)
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
            const auto& f_min = unit.params.upsilon_min;
            const auto& f_max = unit.params.upsilon_max;
            const auto& f_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Initialize the gradient
            grad = core::Vector3r::Zero();

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const core::Vector3r v_rho = x - z;
            const float v_rho_norm = v_rho.norm();
            core::Vector3r rho = core::Vector3r::Zero();
            if (v_rho_norm > 0.001f)
                rho = v_rho / v_rho_norm;

            // Calculate the reference distance to the target
            const float d_ref = r * f_ref / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = r * f_min / s_ref;
            const float U1 = r * f_max / s_ref;
            const float L2 = r * f_min / s_max;
            const float U2 = r * f_max / s_min;

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            // const float lambda_i = 0.0f; // Non-convex term is not considered
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Compute the gradient of the cost function
            // Psi gradient
            float grad_psi = 2.0f *
                (tau0 * max(0.0f, d - U0) * heaviside(d, U0) +
                    tau1 * max(0.0f, d - U1) * heaviside(d, U1) +
                    tau2 * max(0.0f, d - U2) * heaviside(d, U2));
            // Gamma gradient
            core::Vector3r grad_gamma;
            grad_gamma(0) = 2.0f * (x(0) - p_ref(0)) + 2.0f * (d - (x(2) - z(2))) * (x(0) - z(0)) / d;
            grad_gamma(1) = 2.0f * (x(1) - p_ref(1)) + 2.0f * (d - (x(2) - z(2))) * (x(1) - z(1)) / d;
            grad_gamma(2) = 2.0f * (x(2) - p_ref(2)) + 2.0f * (d - (x(2) - z(2))) * ((x(2) - z(2)) / d - 1.0f);
            // Integrate all the gradients
            grad += grad_psi * rho + grad_gamma;

            // Return the value of Ji
            return psi_i + gamma_i;
        }

        static float cameraJ2(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Vector3r& x_hat, const UnitCostParameters& unit, core::Vector3r& grad)
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
            const auto& f_min = unit.params.upsilon_min;
            const auto& f_max = unit.params.upsilon_max;
            const auto& f_ref = unit.params.upsilon_ref;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& sigma0 = unit.sigma0;
            const auto& sigma1 = unit.sigma1;
            const auto& sigma2 = unit.sigma2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Initialize the gradient
            grad = core::Vector3r::Zero();

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const core::Vector3r v = x_hat - z;
            const float v_norm = v.norm();
            core::Vector3r eta = core::Vector3r::Zero();
            if (v_norm > eps_dist)
                eta = v / v_norm;

            const core::Vector3r v_rho = x - z;
            const float v_rho_norm = v_rho.norm();
            core::Vector3r rho = core::Vector3r::Zero();
            if (v_rho_norm > 0.001f)
                rho = v_rho / v_rho_norm;

            // Calculate the projection as a substitute for distance for the non-convex term
            const float d_proj = (x - z).transpose() * eta;

            // Calculate the reference distance to the target
            const float d_ref = r * f_ref / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = r * f_min / s_ref;
            const float U1 = r * f_max / s_ref;
            const float L2 = r * f_min / s_max;
            const float U2 = r * f_max / s_min;

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
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

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
            core::Vector3r grad_gamma;
            grad_gamma(0) = 2.0f * (x(0) - p_ref(0)) + 2.0f * (d - (x(2) - z(2))) * (x(0) - z(0)) / d;
            grad_gamma(1) = 2.0f * (x(1) - p_ref(1)) + 2.0f * (d - (x(2) - z(2))) * (x(1) - z(1)) / d;
            grad_gamma(2) = 2.0f * (x(2) - p_ref(2)) + 2.0f * (d - (x(2) - z(2))) * ((x(2) - z(2)) / d - 1.0f);
            // Integrate all the gradients
            grad += grad_psi * rho + grad_lambda * eta + grad_gamma;

            // Return the value of Ji
            return psi_i + lambda_i + gamma_i;
        }

        static float windowJ1(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Matrix4r& T, const UnitCostParameters& unit, core::Vector3r& grad)
        {
            // Cost function without non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // T: C0 in world frame (frame of central camera)
            // unit: parameters for the cost function for the observation unit
            // grad: gradient of the cost function to be computed

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& lambda_min = unit.params.upsilon_min;
            const auto& lambda_max = unit.params.upsilon_max;
            const auto& lambda_ref = unit.params.upsilon_ref;
            const auto& f = unit.params.window_params.f_ref;
            const auto& full_width = unit.params.window_params.full_width;
            const auto& full_height = unit.params.window_params.full_height;
            const auto& rho_x = unit.params.rho_x;
            const auto& rho_y = unit.params.rho_y;
            const auto& K = unit.params.camera_params.K;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Initialize the gradient
            grad = core::Vector3r::Zero();

            // Project target position onto central camera
            core::Vector2r p = core::VisionUtils::projectPoint(z, T, K);

            // Calculate the correction factor for uncentered targets
            float u_pix = full_width / 2.0f;
            float v_pix = full_height / 2.0f;
            float l = std::sqrt(std::pow(p(0) - u_pix, 2) * std::pow(rho_x, 2) + std::pow(p(1) - v_pix, 2) * std::pow(rho_y, 2));
            float xi = std::sqrt(std::pow(f, 2) + std::pow(l, 2));

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const core::Vector3r v_rho = x - z;
            const float v_rho_norm = v_rho.norm();
            core::Vector3r rho = core::Vector3r::Zero();
            if (v_rho_norm > 0.001f)
                rho = v_rho / v_rho_norm;

            // Calculate the reference distance to the target
            const float d_ref = (r * lambda_ref * xi) / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = (r * lambda_min * xi) / s_ref;
            const float U1 = (r * lambda_max * xi) / s_ref;
            const float L2 = (r * lambda_min * xi) / s_max;
            const float U2 = (r * lambda_max * xi) / s_min;

            // Calculate the index terms based on intervals
            const float psi_i =
                tau0 * pow(max(0.0f, d - U0), 2) +
                tau1 * pow(max(0.0f, d - U1), 2) +
                tau2 * pow(max(0.0f, d - U2), 2);
            // const float lambda_i = 0.0f; // Non-convex term is not considered
            const float gamma_i =
                mu * (x - p_ref).transpose() * (x - p_ref) +
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

            // Compute the gradient of the cost function
            // Psi gradient
            float grad_psi = 2.0f *
                (tau0 * max(0.0f, d - U0) * heaviside(d, U0) +
                    tau1 * max(0.0f, d - U1) * heaviside(d, U1) +
                    tau2 * max(0.0f, d - U2) * heaviside(d, U2));
            // Gamma gradient
            core::Vector3r grad_gamma;
            grad_gamma(0) = 2.0f * (x(0) - p_ref(0)) + 2.0f * (d - (x(2) - z(2))) * (x(0) - z(0)) / d;
            grad_gamma(1) = 2.0f * (x(1) - p_ref(1)) + 2.0f * (d - (x(2) - z(2))) * (x(1) - z(1)) / d;
            grad_gamma(2) = 2.0f * (x(2) - p_ref(2)) + 2.0f * (d - (x(2) - z(2))) * ((x(2) - z(2)) / d - 1.0f);
            // Integrate all the gradients
            grad += grad_psi * rho + grad_gamma;

            // Return the value of Ji
            return psi_i + gamma_i;
        }

        static float windowJ2(const core::Vector3r& z, const float& r, const core::Vector3r& x, const core::Vector3r& x_hat, const core::Matrix4r& T, const UnitCostParameters& unit, core::Vector3r& grad)
        {
            // Cost function with convex relaxation of the non-convex term
            // Args:
            // z: center of the cluster
            // r: radius of the cluster
            // x: position of the vehicle
            // x_hat: estimated position of the vehicle
            // T: C0 in world frame (frame of central camera)
            // unit: parameters for the cost function for the observation unit
            // grad: gradient of the cost function to be computed

            // Distance threshold to consider that x_hat coincides with zi
            // Considering that hMin is several meters, it should not be reached unless set very high, since zi are at height 0
            float eps_dist = 0.1f;

            // Extract cost function parameters
            const auto& s_min = unit.params.s_min;
            const auto& s_max = unit.params.s_max;
            const auto& s_ref = unit.params.s_ref;
            const auto& lambda_min = unit.params.upsilon_min;
            const auto& lambda_max = unit.params.upsilon_max;
            const auto& lambda_ref = unit.params.upsilon_ref;
            const auto& f = unit.params.window_params.f_ref;
            const auto& full_width = unit.params.window_params.full_width;
            const auto& full_height = unit.params.window_params.full_height;
            const auto& rho_x = unit.params.rho_x;
            const auto& rho_y = unit.params.rho_y;
            const auto& K = unit.params.camera_params.K;
            const auto& tau0 = unit.tau0;
            const auto& tau1 = unit.tau1;
            const auto& tau2 = unit.tau2;
            const auto& sigma0 = unit.sigma0;
            const auto& sigma1 = unit.sigma1;
            const auto& sigma2 = unit.sigma2;
            const auto& mu = unit.mu;
            const auto& nu = unit.nu;

            // Initialize the gradient
            grad = core::Vector3r::Zero();

            // Project target position onto central camera
            core::Vector2r p = core::VisionUtils::projectPoint(z, T, K);

            // Calculate the correction factor for uncentered targets
            float u_pix = full_width / 2.0f;
            float v_pix = full_height / 2.0f;
            float l = std::sqrt(std::pow(p(0) - u_pix, 2) * std::pow(rho_x, 2) + std::pow(p(1) - v_pix, 2) * std::pow(rho_y, 2));
            float xi = std::sqrt(std::pow(f, 2) + std::pow(l, 2));

            // Target position and distance to its camera (approximated by distance to the vehicle)
            const float d = (x - z).norm();

            // Vector indicating the direction to project
            const core::Vector3r v = x_hat - z;
            const float v_norm = v.norm();
            core::Vector3r eta = core::Vector3r::Zero();
            if (v_norm > eps_dist)
                eta = v / v_norm;

            const core::Vector3r v_rho = x - z;
            const float v_rho_norm = v_rho.norm();
            core::Vector3r rho = core::Vector3r::Zero();
            if (v_rho_norm > 0.001f)
                rho = v_rho / v_rho_norm;

            // Calculate the projection as a substitute for distance for the non-convex term
            const float d_proj = (x - z).transpose() * eta;

            // Calculate the reference distance to the target
            const float d_ref = (r * lambda_ref * xi) / s_ref;

            // Calculate what would be the ideal reference position in the case of a single target (perfect verticallity)
            core::Vector3r p_ref = z;
            p_ref(2) += d_ref;

            // Determine the nested intervals
            const float L0 = d_ref;
            const float U0 = d_ref;
            const float L1 = (r * lambda_min * xi) / s_ref;
            const float U1 = (r * lambda_max * xi) / s_ref;
            const float L2 = (r * lambda_min * xi) / s_max;
            const float U2 = (r * lambda_max * xi) / s_min;

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
                nu * pow((d - (x - z).transpose() * core::Vector3r(0.0f, 0.0f, 1.0f)), 2);

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
            core::Vector3r grad_gamma;
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

} // namespace flychams::coordination