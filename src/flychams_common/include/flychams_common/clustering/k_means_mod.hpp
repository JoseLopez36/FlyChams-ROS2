#pragma once

// Standard includes
#include <vector>
#include <algorithm>

/* Hungarian algorithm: https://github.com/mcximing/hungarian-algorithm-cpp */
#include "Hungarian.h"

// Core includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/utils/math_utils.hpp"

namespace flychams::common
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief K-Means modified implementation
	 *
	 * @details
	 * This class implements a K-Means modified algorithm.
	 * It provides methods for clustering targets.
	 * The modifications include:
	 * - Inter-iteration cluster consistency.
	 * - Time persistence of clusters.
	 *
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-01-29
	 * ════════════════════════════════════════════════════════════════
	 */
	class KMeansMod
	{
	public: // Types
		using SharedPtr = std::shared_ptr<KMeansMod>;
		// Modes
		enum class Mode
		{
			INITIAL,
			CONSISTENT_AND_PERSISTENT,
		};
		// Parameters
        struct Parameters
        {
            // Persistence parameters
            float ini_bonding_coef;
            float max_bonding_coef;
            float bonding_coef_time_to_max;
            float max_hysteresis_ratio;
            float min_hysteresis_ratio;
            float change_cooldown_time;
        };

	private: // Parameters
		Parameters params_;

	private: // Data
		// Persistence data
		common::RowVectorXr bonding_coefs_;
		float change_cooldown_remaining_;

    public: // Public methods
        void init(const Parameters& params)
        {
            // Store parameters
            params_ = params;

			// Initialize persistence data
			bonding_coefs_.resize(0);
			change_cooldown_remaining_ = 0.0f;
        }
        void destroy()
        {
            // Nothing to destroy
        }
        common::RowVectorXi run(const int& K, const common::Matrix3Xr& tab_P, const common::RowVectorXi& C_prev, const Mode& mode, const float& dt)
        {
			// Get number of points
			int n = tab_P.cols();

			// Check edge cases
			if (K <= 0 || n < K || C_prev.size() != n)
			{
				throw std::invalid_argument("Invalid input parameters");
			}

			// Check if we need to reinitialize persistence data
			if (bonding_coefs_.size() != n)
			{
				bonding_coefs_.resize(n);
				bonding_coefs_.setConstant(params_.ini_bonding_coef);
			}

			// Perform clustering with different algorithms, depending on mode
			common::RowVectorXi C(n);
			switch (mode)
			{
				case Mode::INITIAL:
				{
					// Perform clustering with only base K-means:
					C = computeInitialAssignments(K, tab_P);
					break;
				}

				case Mode::CONSISTENT_AND_PERSISTENT:
				{
					// Perform clustering with modified K-means:
					// 1. Perform clustering with base K-means
					C = computeInitialAssignments(K, tab_P);

					// 2. Compute current and previous centroids
					common::Matrix3Xr centroids = computeCentroids(C, tab_P);
					common::Matrix3Xr centroids_prev = computeCentroids(C_prev, tab_P);

					// 3. Ensure consistency in cluster numbering
					C = reorderCentroidsConsistently(C, centroids, centroids_prev);
					centroids = computeCentroids(C, tab_P); // We need to recompute the centroids after reordering

					// Block successive changes while the cooldown is active
					if (change_cooldown_remaining_ > 0.0f)
					{
						change_cooldown_remaining_ = std::max(change_cooldown_remaining_ - dt, 0.0f);
						C = C_prev;
						bonding_coefs_.setConstant(params_.max_bonding_coef);
						break;
					}

					// 4. Ensure persistence of centroids
					common::RowVectorXi C_candidate = C;
					bool persistence_broken = false;
					C = ensureClusterPersistence(C, C_prev, centroids, tab_P, dt, persistence_broken);
					if (persistence_broken)
					{
						C = C_candidate;
					}
					if ((C.array() != C_prev.array()).any())
					{
						bonding_coefs_.setConstant(params_.max_bonding_coef);
						change_cooldown_remaining_ = params_.change_cooldown_time;
					}
					break;
				}

				default:
					throw std::invalid_argument("Invalid clustering mode");
			}

			// Return assignments
			return C;
        }

	private: // Base K-means implementation. Initial assignments
		common::RowVectorXi computeInitialAssignments(const int& K, const common::Matrix3Xr& tab_P)
		{
			// Get number of points
			int n = tab_P.cols();

			// Function that computes the initial assignments for the base K-means algorithm
			common::RowVectorXi C = common::RowVectorXi::Constant(n, -1);

			// Initialize clusters with the K farthest points
			common::RowVectorXi farthest = selectFarthestPoints(K, tab_P);
			for (int k = 0; k < K; k++)
			{
				C(farthest(k)) = k;
			}

			// K-means Iteration
			bool has_changed = true;
			common::RowVectorXi C_prev = C;
			while (has_changed)
			{
				// Compute new centroids
				common::Matrix3Xr centroids = computeCentroids(C, tab_P);

				// Store calculated assignments to check for changes
				C_prev = C;

				// Assign clusters based on the updated centroids
				C = assignClusters(centroids, tab_P);

				// Check if any assignments have changed
				has_changed = (C.array() != C_prev.array()).any();

				// If it hasn't changed, return the last result
			}

			return C;
		}

		common::RowVectorXi selectFarthestPoints(const int& K, const common::Matrix3Xr& tab_P)
		{
			// Function that selects the 'K' farthest points from a given set of points provided by the
			// columns of 'points'. 
			// Get number of points
			int n = tab_P.cols();

			// If 'K' is 1, it simply selects the first point.
			if (K == 1)
			{
				common::RowVectorXi L(1);
				L << 0;
				return L;
			}

			// Calculate the distance matrix between points
			common::MatrixXr D = computeDistanceMatrix(tab_P);

			// Find the indices of the two most separated points
			common::RowVectorXr max_vals(n);
			common::RowVectorXi max_indices(n);
			int max_col;
			for (int i = 0; i < n; i++)
			{
				max_vals(i) = D.col(i).maxCoeff(&max_indices(i));
			}
			max_vals.maxCoeff(&max_col);
			int idx1 = max_indices(max_col);
			int idx2 = max_col;

			// Initially, the selected list contains the two most distant points
			int count = 2;
			common::RowVectorXi L(count);
			L << idx1, idx2;
			while (count < K)
			{
				int max_min_index = 0;
				float max_min_d = 0.0f;
				for (int i = 0; i < n; i++)
				{
					// If the i-th point is not already selected
					if ((L.array() != i).all())
					{
						// Find the minimum distance of this point to any of the selected points
						float min_d = HUGE_VALF;
						for (int j = 0; j < count; j++)
						{
							float d = D(i, L(j));
							if (d < min_d)
							{
								min_d = d;
							}
						}
						if (min_d > max_min_d)
						{
							max_min_d = min_d;
							max_min_index = i;
						}
					}
				}

				// Add the farthest point to the list
				count++;
				common::RowVectorXi L_new(count);
				L_new << L, max_min_index;
				L = L_new;
			}

			return L;
		}

		common::MatrixXr computeDistanceMatrix(const common::Matrix3Xr& tab_P)
		{
			// Function that computes the distance matrix between each pair of points in a set.
			// Get number of points
			int n = tab_P.cols();

			// Compute distance matrix
			common::MatrixXr D = common::MatrixXr::Zero(n, n);
			float d;
			for (int i = 0; i < n; i++)
			{
				for (int j = i + 1; j < n; j++)
				{
					d = (tab_P.col(i) - tab_P.col(j)).norm();
					D(i, j) = d;
					D(j, i) = d;
				}
			}

			return D;
		}

		common::Matrix3Xr computeCentroids(const common::RowVectorXi& C, const common::Matrix3Xr& tab_P)
		{
			// Function that computes the centroids of the clusters in the given assignments C.
			// Get number of points and clusters
			int n = tab_P.cols();
			int K = C.maxCoeff() + 1;

			// Initialize centroids and number of points per cluster
			common::RowVectorXi nk = common::RowVectorXi::Zero(K);
			common::Matrix3Xr centroids = common::Matrix3Xr::Zero(3, K);

			// Sum all points for each cluster
			for (int i = 0; i < n; i++)
			{
				int c = C(i);
				if (c >= 0 && c < K)
				{
					centroids.col(c) += tab_P.col(i);
					nk(c)++;
				}
			}

			// Compute average for each centroid
			for (int k = 0; k < K; k++)
			{
				float n_points = static_cast<float>(nk(k));
				if (n_points > 0.0f)
				{
					centroids.col(k) /= n_points;
				}
			}

			return centroids;
		}

		common::RowVectorXi assignClusters(const common::Matrix3Xr& centroids, const common::Matrix3Xr& tab_P)
		{
			// Get number of points and clusters
			int n = tab_P.cols();
			int K = centroids.cols();

			// Initialize assignments
			common::RowVectorXi C = common::RowVectorXi::Constant(n, -1);

			for (int i = 0; i < n; i++)
			{
				float min_d = HUGE_VALF;
				int best_k = 0;

				for (int k = 0; k < K; k++)
				{
					float d = (tab_P.col(i) - centroids.col(k)).norm();
					if (d < min_d)
					{
						min_d = d;
						best_k = k;
					}
				}

				C(i) = best_k;
			}

			return C;
		}

	private: // Consistency implementation. Consistent centroids
		common::RowVectorXi reorderCentroidsConsistently(const common::RowVectorXi& C, const common::Matrix3Xr& centroids, const common::Matrix3Xr& centroids_prev)
		{
			// Get number of points and clusters
			int n = C.size();
			int K = centroids.cols();

			// Function that reorders the centroids to ensure consistency in cluster numbering.
			common::RowVectorXi C_consistent = common::RowVectorXi::Constant(n, -1);

			// Ensure consistency in cluster numbering
			common::RowVectorXi associations = associateCentroids(centroids, centroids_prev);

			// Update point assignments based on reordered centroids
			for (int i = 0; i < n; i++)
			{
				int new_index = 0;
				for (int j = 0; j < K; j++)
				{
					if (associations(j) == C(i))
					{
						new_index = j;
						break;
					}
				}
				C_consistent(i) = new_index;
			}

			return C_consistent;
		}

		common::RowVectorXi associateCentroids(const common::Matrix3Xr& centroids, const common::Matrix3Xr& centroids_prev)
		{
			// Function that associates the centroids to ensure consistency in cluster numbering.
			// Get number of clusters
			int K = centroids.cols();

			// Initialize associations
			common::RowVectorXi associations = common::RowVectorXi::Constant(K, -1);

			// Calculate the distance matrix between elements of both groups
			common::MatrixXr D = computeDistanceMatrixTwoGroups(centroids, centroids_prev);

			// Hungarian algorithm
			// Convert the Eigen distance matrix to a vector of vectors for the Hungarian Algorithm
			std::vector<std::vector<double>> cost_matrix(K, std::vector<double>(K));
			for (int i = 0; i < K; i++)
			{
				for (int j = 0; j < K; j++)
				{	
					cost_matrix[i][j] = static_cast<double>(D(i, j));
				}
			}

			// Execute the Hungarian Algorithm
			HungarianAlgorithm hung_algo_object;
			std::vector<int> assigned_indices;
			hung_algo_object.Solve(cost_matrix, assigned_indices);

			// Convert the assignment result to a RowVectorXi
			for (int k = 0; k < K; k++)
			{
				associations(k) = assigned_indices[k];
			}

			return associations;
		}

		common::MatrixXr computeDistanceMatrixTwoGroups(const common::Matrix3Xr& centroids, const common::Matrix3Xr& centroids_prev)
		{
			// Function that provides the distance matrix between points of two groups.
			// Unlike 'computeDistanceMatrix()', which calculated distances between all points in a single set
			// Get number of clusters
			int K = centroids.cols();

			// Initialize distance matrix
			common::MatrixXr D = common::MatrixXr::Zero(K, K);

			// Calculate squared Euclidean distances
			for (int i = 0; i < K; i++)
			{
				for (int j = 0; j < K; j++)
				{
					D(i, j) = (centroids_prev.col(i) - centroids.col(j)).squaredNorm();
				}
			}

			return D;
		}

	private: // Persistence implementation. Cluster persistence
		common::RowVectorXi ensureClusterPersistence(const common::RowVectorXi& C, const common::RowVectorXi& C_prev, const common::Matrix3Xr& centroids, const common::Matrix3Xr& tab_P, const float& dt, bool& persistence_broken)
	    {
			// Function that ensures the persistence of centroids over time
			// Get number of points
			int n = C.size();

			// Initialize assignments with input assignments
			common::RowVectorXi C_persistent = C;

			// Check and fix parameters
			if (params_.ini_bonding_coef > params_.max_bonding_coef)
			{
				params_.ini_bonding_coef = params_.max_bonding_coef;
			}
			if (params_.min_hysteresis_ratio > params_.max_hysteresis_ratio)
			{
				params_.min_hysteresis_ratio = params_.max_hysteresis_ratio / 2.0f;
			}

			// Calculate the slope for incrementing the bonding coefficient
			float bonding_coef_incr_slope = params_.max_bonding_coef / params_.bonding_coef_time_to_max;

			// For each point, check if its assignment has changed
			for (int i = 0; i < n; i++)
			{
				int c_prev = C_prev(i);
				int c = C_persistent(i);

				// If the point remains in the same cluster
				if (c == c_prev)
				{
					// Increase the bonding coefficient
					bonding_coefs_(i) += bonding_coef_incr_slope * dt;

					// Enforce maximum bonding coefficient
					bonding_coefs_(i) = std::min(bonding_coefs_(i), params_.max_bonding_coef);
				}
				else
				{
					// Decrease the bonding coefficient
					bonding_coefs_(i) -= bonding_coef_incr_slope * dt;

					// Enforce minimum bonding coefficient
					bonding_coefs_(i) = std::max(bonding_coefs_(i), 0.0f);

					// Calculate dynamic hysteresis threshold for distance measurements
					float hysteresis_ratio = params_.min_hysteresis_ratio + bonding_coefs_(i) * (params_.max_hysteresis_ratio - params_.min_hysteresis_ratio) / params_.max_bonding_coef;

					// Calculate where the centroid of the previous cluster would be 
					// if this point continued to be associated with it
					common::Vector3r centroid_prev_with_point = calculateCentroidWithInclusion(C_persistent, c_prev, tab_P, i);

					// Calculate distances
					float dist_to_prev_cluster = (tab_P.col(i) - centroid_prev_with_point).norm();
					float dist_to_curr_cluster = (tab_P.col(i) - centroids.col(c)).norm();
					float dist_between_clusters = (centroid_prev_with_point - centroids.col(c)).norm();

					// If the difference doesn't exceed the current hysteresis threshold,
					// revert the point to its previous cluster assignment
					if ((dist_to_prev_cluster - dist_to_curr_cluster) / dist_between_clusters <= hysteresis_ratio)
					{
						C_persistent(i) = c_prev;
					}
					else
					{
						// If the point should change to the new cluster, reset its bonding coefficient
						bonding_coefs_(i) = params_.ini_bonding_coef;
						persistence_broken = true;
					}
				}
			}

			return C_persistent;
		}

		common::Vector3r calculateCentroidWithInclusion(const common::RowVectorXi& C, const int& c, const common::Matrix3Xr& tab_P, const int& p)
		{
			// Function that calculates the centroid of a cluster including a specific point.
			// Get number of points
			int n = tab_P.cols();

			// Sum all points belonging to the cluster
			common::Vector3r centroid = common::Vector3r::Zero();
			int count = 0;
			for (int i = 0; i < n; i++)
			{
				if (C(i) == c || i == p)
				{
					centroid += tab_P.col(i);
					count++;
				}
			}

			// Calculate average
			if (count > 0)
			{
				centroid /= static_cast<float>(count);
			}

			return centroid;
		}
	};

} // namespace flychams::common
