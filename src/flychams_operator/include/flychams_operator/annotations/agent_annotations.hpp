#pragma once

// Utils include
#include "flychams_operator/annotations/annotation_parameters.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-agent image annotation generator for Foxglove
     *
     * @details
     * Subscribes to ObservationSetpoints for the agent. On each timer
     * tick it publishes one foxglove_msgs/ImageAnnotations message per
     * observation unit. The annotation color is derived from the agent's
     * palette colour via the mission settings idx.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-23
     * ════════════════════════════════════════════════════════════════
     */
    class AgentAnnotations : public common::BaseModule
    {
    public: // Constructor/Destructor
        AgentAnnotations(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentAnnotations>;
        struct Intrinsics
        {
            double fx = 0.0, fy = 0.0, cx = 0.0, cy = 0.0;
            bool valid = false;
        };

    private: // Parameters
        common::ID agent_id_;
        int agent_idx_ = 0;
        float update_rate_;
        // View dimensions
        int central_view_width_;
        int central_view_height_;
        int tracking_view_width_;
        int tracking_view_height_;
        // Per-camera original resolution (for cluster projection scaling)
        std::unordered_map<common::ID, std::pair<int, int>> original_resolutions_;

    private: // Setpoints data
        common::ObservationSetpointsMsg::SharedPtr setpoints_;
        bool has_setpoints_ = false;

    private: // Cluster data
        struct ClusterData
        {
            common::PointMsg center;
            float radius = 0.0f;
            common::ID unit_id;
        };
        std::vector<ClusterData> clusters_;
        bool has_clusters_ = false;

    private: // Per-camera intrinsics cache
        std::unordered_map<common::ID, Intrinsics> intrinsics_;

    private: // Callbacks
        void observationSetpointsCallback(const common::ObservationSetpointsMsg::SharedPtr msg);
        void clustersCallback(const common::AgentClustersMsg::SharedPtr msg);
        void cameraInfoCallback(const common::ID& camera_id, const common::CameraInfoMsg::SharedPtr msg);

    private: // Update methods
        void update();

    private: // Annotation helpers
        void publishCameraAnnotations(size_t idx, int view_w, int view_h) const;
        void publishWindowAnnotations(size_t idx, int view_w, int view_h) const;
        void appendClusterOverlays(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const common::ID& camera_id, int view_w, int view_h, bool only_show_assigned, float scale) const;
        void appendClusterOverlaysWindow(common::FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const common::ID& camera_id, size_t sp_idx, int view_w, int view_h, bool only_show_assigned, float scale) const;
        common::Matrix4r buildWTc(const common::TransformMsg& tf) const;
        common::Matrix3r buildK(const Intrinsics& intr) const;
        std::vector<common::FoxPoint2Msg> projectRim(const common::Vector3r& wP, float radius, const common::Matrix4r& wTc, const common::Matrix3r& K, int n_pts = 64) const;

    private: // ROS components
        common::TimerPtr update_timer_;
        // Per-unit publishers: keyed by observation unit ID
        std::unordered_map<common::ID, common::PublisherPtr<common::FoxImageAnnotationsMsg>> annotation_pubs_;
        // Subscribers
        common::SubscriberPtr<common::ObservationSetpointsMsg> setpoints_sub_;
        common::SubscriberPtr<common::AgentClustersMsg> clusters_sub_;
        // Per-camera camera_info subscribers (keyed by camera_id)
        std::unordered_map<common::ID, common::SubscriberPtr<common::CameraInfoMsg>> camera_info_subs_;
    };

} // namespace flychams::operator_pkg