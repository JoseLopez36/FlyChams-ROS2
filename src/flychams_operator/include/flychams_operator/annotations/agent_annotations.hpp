#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Module that publishes image annotations for each
     *        observation unit based on its current setpoints
     *
     * @details
     * Subscribes to ObservationSetpoints for the agent. On each timer
     * tick it publishes one foxglove_msgs/ImageAnnotations message per
     * observation unit containing:
     *
     *   Camera units
     *   - A crosshair (two intersecting lines) at image centre showing
     *     the locked aim point
     *   - A text annotation showing zoom factor and rotation
     *
     *   Window units
     *   - A rectangle outline drawn from the crop corners
     *   - Corner tick marks for visual clarity
     *   - A text annotation showing the crop dimensions and zoom factor
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-18
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
        common::ID element_id_;
        float update_rate_;
        // View dimensions
        int central_view_width_;
        int central_view_height_;
        int tracking_view_width_;
        int tracking_view_height_;

    private: // Setpoints data
        common::ObservationSetpointsMsg::SharedPtr setpoints_;
        bool has_setpoints_ = false;

    private: // Callbacks
        void observationSetpointsCallback(const common::ObservationSetpointsMsg::SharedPtr msg);

    private: // Update methods
        void update();
        bool isDataValid() const;

    private: // Annotation helpers
        void publishCameraAnnotations(size_t idx, int view_w, int view_h) const;
        void publishWindowAnnotations(size_t idx, int view_w, int view_h) const;

    private: // ROS components
        common::TimerPtr update_timer_;
        // Per-unit publishers: indexed by observation unit position in setpoints
        std::vector<common::PublisherPtr<common::FoxImageAnnotationsMsg>> annotation_pubs_;
        // Subscriber
        common::SubscriberPtr<common::ObservationSetpointsMsg> setpoints_sub_;
    };

} // namespace flychams::operator_pkg