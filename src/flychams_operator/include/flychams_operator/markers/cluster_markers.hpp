#pragma once

// Utils include
#include "flychams_operator/markers/marker_utils.hpp"

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
     * Publishes a foxglove_msgs/SceneUpdate with:
     *   - Transparent volume sphere
     *   - Solid equatorial ring
     *   - Dashed radius line
     *   - Text label with ID and radius
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-18
     * ════════════════════════════════════════════════════════════════
     */
    class ClusterMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        ClusterMarkers(const common::ID& cluster_id, const common::ID& element_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), cluster_id_(cluster_id), element_id_(element_id)
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
            common::PointMsg center;
            float radius = 0.0f;
            bool has_geometry = false;
        };

    private: // Parameters
        common::ID cluster_id_;
        common::ID element_id_;
        float update_rate_;

    private: // Data
        ClusterData cluster_;

    private: // Callbacks
        void geometryCallback(const common::ClusterGeometryMsg::SharedPtr msg);

    private: // Update
        void update();
        bool isDataValid() const;

    private: // ROS components
        common::TimerPtr update_timer_;
        common::PublisherPtr<common::FoxSceneUpdateMsg> scene_pub_;
        common::SubscriberPtr<common::ClusterGeometryMsg> geometry_sub_;
    };

} // namespace flychams::operator_pkg