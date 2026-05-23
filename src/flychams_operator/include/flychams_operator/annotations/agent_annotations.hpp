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

    private: // Parameters
        common::ID agent_id_;
        int agent_idx_ = 0;
        float update_rate_;
        // View dimensions
        int central_view_width_;
        int central_view_height_;
        int tracking_view_width_;
        int tracking_view_height_;
        // Central camera resolution
        int original_view_width_;
        int original_view_height_;

    private: // Setpoints data
        common::ObservationSetpointsMsg::SharedPtr setpoints_;
        bool has_setpoints_ = false;

    private: // Callbacks
        void observationSetpointsCallback(const common::ObservationSetpointsMsg::SharedPtr msg);

    private: // Update methods
        void update();

    private: // Annotation helpers
        void publishCameraAnnotations(size_t idx, int view_w, int view_h) const;
        void publishWindowAnnotations(size_t idx, int view_w, int view_h) const;

    private: // ROS components
        common::TimerPtr update_timer_;
        // Per-unit publishers: keyed by observation unit ID
        std::unordered_map<common::ID, common::PublisherPtr<common::FoxImageAnnotationsMsg>> annotation_pubs_;
        // Subscriber
        common::SubscriberPtr<common::ObservationSetpointsMsg> setpoints_sub_;
    };

} // namespace flychams::operator_pkg