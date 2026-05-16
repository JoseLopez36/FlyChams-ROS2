#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-cluster marker publisher for Foxglove visualization
     *
     * @details
     * Subscribes to a cluster's geometry topic. On each timer tick it
     * publishes a MarkerArray containing a SPHERE marker representing
     * the cluster bounding sphere and a LINE_LIST marker tracing
     * its radius, plus a TEXT marker with the cluster ID.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class ClusterMarkers : public core::BaseModule
    {
    public: // Constructor/Destructor
        ClusterMarkers(const core::ID& cluster_id, core::BaseNode::SharedPtr node)
            : BaseModule(node), cluster_id_(cluster_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<ClusterMarkers>;
        struct ClusterData
        {
            // Latest geometry
            core::PointMsg center;
            float radius;
            bool has_geometry;
            // Publisher
            core::PublisherPtr<core::MarkerArrayMsg> markers_pub;
            // Subscribers
            core::SubscriberPtr<core::ClusterGeometryMsg> geometry_sub;
            // Constructor
            ClusterData()
                : center(), radius(0.0f), has_geometry(false), markers_pub(), geometry_sub()
            {
            }
        };

    private: // Parameters
        core::ID cluster_id_;
        float update_rate_;

    private: // Data
        ClusterData cluster_;

    private: // Callbacks
        void clusterGeometryCallback(const core::ClusterGeometryMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        core::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg