#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-target marker publisher for Foxglove visualization
     *
     * @details
     * Subscribes to a target's position topic. On each timer tick it
     * publishes a MarkerArray containing a CYLINDER marker representing
     * the target body and a TEXT marker with its ID.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class TargetMarkers : public core::BaseModule
    {
    public: // Constructor/Destructor
        TargetMarkers(const core::ID& target_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), target_id_(target_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<TargetMarkers>;
        struct TargetData
        {
            // Latest position
            core::PointMsg position;
            bool has_position;
            // Publisher
            core::PublisherPtr<core::MarkerArrayMsg> markers_pub;
            // Subscribers
            core::SubscriberPtr<core::PointStampedMsg> position_sub;
            // Constructor
            TargetData()
                : position(), has_position(false), markers_pub(), position_sub()
            {
            }
        };

    private: // Parameters
        core::ID target_id_;
        float update_rate_;

    private: // Data
        TargetData target_;

    private: // Callbacks
        void positionCallback(const core::PointStampedMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        core::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg