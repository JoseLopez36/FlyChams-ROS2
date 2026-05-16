#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

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
    class TargetMetrics : public common::BaseModule
    {
    public: // Constructor/Destructor
        TargetMetrics(const common::ID& target_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), target_id_(target_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<TargetMetrics>;
        struct TargetData
        {
            // Latest position
            common::PointMsg position;
            bool has_position;
            // Publisher
            common::PublisherPtr<common::TargetMetricsMsg> metrics_pub;
            // Subscribers
            common::SubscriberPtr<common::PointStampedMsg> position_sub;
            // Constructor
            TargetData()
                : position(), has_position(false), metrics_pub(), position_sub()
            {
            }
        };

    private: // Parameters
        common::ID target_id_;
        float update_rate_;

    private: // Accumulated data
        TargetData target_;
        common::PointMsg last_position_;
        float distance_traveled_;
        float total_speed_;
        int speed_samples_;
        common::Time last_update_time_;
        float time_elapsed_;

    private: // Callbacks
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg