#pragma once

// Utils include
#include "flychams_simulation/bridge/simulation_bridge.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Controller for targets and clusters in the simulation
     *
     * @details
     * This class is responsible for managing the control of targets and
     * clusters in the simulation. It handles the control of the target
     * and its clusters, mainly their instantaneus position.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-27
     * ════════════════════════════════════════════════════════════════
     */
    class TargetControl : public common::BaseModule
    {
    public: // Constructor/Destructor
        TargetControl(common::BaseNode::SharedPtr node)
            : BaseModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<TargetControl>;
        struct Target
        {
            // Position
            common::PointMsg position;
            bool has_position;
            // Subscriber
            common::SubscriberPtr<common::PointStampedMsg> position_sub;
            // Constructor
            Target()
                : position(), has_position(false), position_sub()
            {
            }
        };
        struct Cluster
        {
            // Geometry
            common::PointMsg position;
            float radius;
            bool has_geometry;
            // Subscriber
            common::SubscriberPtr<common::ClusterGeometryMsg> geometry_sub;
            // Constructor
            Cluster()
                : position(), radius(), has_geometry(false), geometry_sub()
            {
            }
        };

    private: // Parameters
        float update_rate_;
        bool highlight_targets_;
        bool highlight_clusters_;

    private: // Data
        // Targets
        std::unordered_map<common::ID, Target> targets_;
        // Clusters
        std::unordered_map<common::ID, Cluster> clusters_;
        // Other
        int spawn_index_;
        // Simulation tools
        SimulationBridge::SharedPtr simulation_tools_;

    public: // Public methods
        void addCluster(const common::ID& cluster_id);
        void addTarget(const common::ID& target_id);
        void removeCluster(const common::ID& cluster_id);
        void removeTarget(const common::ID& target_id);

    private: // Callbacks
        void targetPositionCallback(const common::ID& target_id, const common::PointStampedMsg::SharedPtr msg);
        void clusterGeometryCallback(const common::ID& cluster_id, const common::ClusterGeometryMsg::SharedPtr msg);

    private: // Control management
        void update();
        bool checkStatus();

    private: // Control methods
        void destroyTargets();
        void destroyClusters();
        void spawnTarget(const common::ID& target_id, const common::PointMsg& initial_position, const common::TargetType& target_type);
        void spawnCluster(const common::ID& cluster_id, const common::PointMsg& initial_center, const float& initial_radius);
        void updateTargets();
        void updateClusters();

    private:
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::simulation