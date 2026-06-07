#pragma once

// Standard includes
#include <vector>
#include <algorithm>
#include <numeric>

// Position solver
#include "flychams_common/positioning/position_solver.hpp"

// Utilities
#include "flychams_common/types/core_types.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Solver for agent assignment using greedy nearest-cluster
     *
     * @details
     * Assigns clusters to agents greedily by Euclidean distance.
     * Each agent is processed in order of its distance to its nearest
     * available cluster. For each agent, the nk closest unassigned
     * clusters are selected. No optimisation is performed — this
     * serves as a fast baseline and default fallback mode.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-06-04
     * ════════════════════════════════════════════════════════════════
     */
    class Greedy
    {
    public: // Types
        // Parameters
        struct Parameters
        {
            // Optimization weights (unused — kept for interface consistency)
            float observation_weight;
            float distance_weight;
            float switch_weight;
        };

    private: // Parameters
        Parameters params_;

    public: // Public methods
        void init(const Parameters& params)
        {
            // Store parameters
            params_ = params;
        }
        void destroy()
        {
            // Nothing to destroy
        }
        std::pair<common::RowVectorXi, int> run(const common::Matrix3Xr& tab_x, const common::Matrix3Xr& tab_P, const common::RowVectorXr& tab_r,
            const common::RowVectorXi& X_prev, std::vector<PositionSolver::SharedPtr>& solvers)
        {
            int m = tab_x.cols();  // number of agents
            int n = tab_P.cols();  // number of clusters

            // nk(k): number of clusters assigned to agent k
            common::RowVectorXi nk = common::RowVectorXi::Zero(m);
            for (int k = 0; k < m; k++)
            {
                nk(k) = solvers[k]->getUnitCount() - 1;
            }

            // Track which clusters are still available
            std::vector<bool> available(n, true);

            // Sort agents by distance to their nearest available cluster (ascending)
            // so the closest agent-cluster pair is resolved first
            std::vector<int> agent_order(m);
            std::iota(agent_order.begin(), agent_order.end(), 0);
            std::sort(agent_order.begin(), agent_order.end(),
                [&](int a, int b)
                {
                    float min_a = HUGE_VALF, min_b = HUGE_VALF;
                    for (int i = 0; i < n; i++)
                    {
                        float d = (tab_x.col(a) - tab_P.col(i)).norm();
                        if (d < min_a) min_a = d;
                        d = (tab_x.col(b) - tab_P.col(i)).norm();
                        if (d < min_b) min_b = d;
                    }
                    return min_a < min_b;
                });

            // Build result vector X in original agent order
            int total_units = nk.sum();
            common::RowVectorXi X = common::RowVectorXi::Zero(total_units);

            // Offset into X for each agent
            std::vector<int> offsets(m, 0);
            for (int k = 1; k < m; k++)
                offsets[k] = offsets[k - 1] + nk(k - 1);

            int node_count = 0;

            for (int a : agent_order)
            {
                // Rank available clusters by distance to this agent
                std::vector<int> cluster_order;
                for (int i = 0; i < n; i++)
                    if (available[i]) cluster_order.push_back(i);

                std::sort(cluster_order.begin(), cluster_order.end(),
                    [&](int i, int j)
                    {
                        return (tab_x.col(a) - tab_P.col(i)).norm() <
                               (tab_x.col(a) - tab_P.col(j)).norm();
                    });

                // Assign the nk(a) nearest available clusters
                int assigned = 0;
                for (int i : cluster_order)
                {
                    if (assigned >= nk(a)) break;
                    X(offsets[a] + assigned) = i;
                    available[i] = false;
                    assigned++;
                    node_count++;  // one distance evaluation per assigned cluster
                }
            }

            return std::make_pair(X, node_count);
        }
    };

} // namespace flychams::common