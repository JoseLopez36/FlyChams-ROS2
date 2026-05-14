#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-target metrics aggregator
     *
     * @details
     * Subscribes to a target's position topic. On each timer tick it
     * computes distance_traveled, speed and average_speed, then
     * publishes a TargetMetrics message.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class TargetMetrics : public core::BaseModule
    {
    public: // Constructor/Destructor
        TargetMetrics(const core::ID& target_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), target_id_(target_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<TargetMetrics>;
        struct TargetData
        {
            // Latest position
            core::PointMsg position;
            bool has_position;
            // Publisher
            core::PublisherPtr<core::TargetMetricsMsg> metrics_pub;
            // Subscribers
            core::SubscriberPtr<core::PointStampedMsg> position_sub;
            // Constructor
            TargetData()
                : position(), has_position(false), metrics_pub(), position_sub()
            {
            }
        };

    private: // Parameters
        core::ID target_id_;
        float update_rate_;

    private: // Accumulated data
        TargetData target_;
        core::PointMsg last_position_;
        float distance_traveled_;
        float total_speed_;
        int speed_samples_;
        core::Time last_update_time_;
        float time_elapsed_;

    private: // Callbacks
        void positionCallback(const core::PointStampedMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        core::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg