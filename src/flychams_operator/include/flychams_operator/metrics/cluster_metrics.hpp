#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

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
    class ClusterMetrics : public core::BaseModule
    {
    public: // Constructor/Destructor
        ClusterMetrics(const core::ID& cluster_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), cluster_id_(cluster_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<ClusterMetrics>;
        struct ClusterData
        {
            // Latest centroid
            core::PointMsg center;
            float radius;
            bool has_geometry;
            // Publisher
            core::PublisherPtr<core::ClusterMetricsMsg> metrics_pub;
            // Subscribers
            core::SubscriberPtr<core::ClusterGeometryMsg> geometry_sub;
            // Constructor
            ClusterData()
                : center(), radius(0.0f), has_geometry(false), metrics_pub(), geometry_sub()
            {
            }
        };

    private: // Parameters
        core::ID cluster_id_;
        float update_rate_;

    private: // Accumulated data
        ClusterData cluster_;
        core::PointMsg last_center_;
        float distance_traveled_;
        float total_speed_;
        int speed_samples_;
        core::Time last_update_time_;
        float time_elapsed_;

    private: // Callbacks
        void clusterGeometryCallback(const core::ClusterGeometryMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        core::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg