#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-cluster metrics aggregator
     *
     * @details
     * Subscribes to a cluster's geometry topic. On each timer tick it
     * computes distance_traveled (centroid), speed and average_speed,
     * then publishes a ClusterMetrics message.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class ClusterMetrics : public common::BaseModule
    {
    public: // Constructor/Destructor
        ClusterMetrics(const common::ID& cluster_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), cluster_id_(cluster_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<ClusterMetrics>;
        struct ClusterData
        {
            // Latest centroid
            common::PointMsg center;
            float radius;
            bool has_geometry;
            // Publisher
            common::PublisherPtr<common::ClusterMetricsMsg> metrics_pub;
            // Subscribers
            common::SubscriberPtr<common::ClusterGeometryMsg> geometry_sub;
            // Constructor
            ClusterData()
                : center(), radius(0.0f), has_geometry(false), metrics_pub(), geometry_sub()
            {
            }
        };

    private: // Parameters
        common::ID cluster_id_;
        float update_rate_;

    private: // Accumulated data
        ClusterData cluster_;
        common::PointMsg last_center_;
        float distance_traveled_;
        float total_speed_;
        int speed_samples_;
        common::Time last_update_time_;
        float time_elapsed_;

    private: // Callbacks
        void clusterGeometryCallback(const common::ClusterGeometryMsg::SharedPtr msg);

    private: // Update
        void update();
        bool checkStatus();

    private: // ROS components
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg