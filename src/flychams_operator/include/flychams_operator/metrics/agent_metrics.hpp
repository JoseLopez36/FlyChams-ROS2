#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-agent metrics aggregator
     *
     * @details
     * Subscribes to an agent's local position, position setpoint,
     * observation setpoints and agent status topics. On each timer
     * tick it computes distance_traveled, speed, distance_to_goal
     * and average_speed, then publishes an AgentMetrics message.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class AgentMetrics : public core::BaseModule
    {
    public: // Constructor/Destructor
        AgentMetrics(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentMetrics>;
        struct AgentData
        {
            // Latest position
            core::PointMsg position;
            bool has_position;
            // Latest setpoint
            core::PointMsg setpoint;
            bool has_setpoint;
            // Latest zoom factors (from observation setpoints)
            std::vector<float> zoom_factors;
            bool has_observation_setpoints;
            // Publisher
            core::PublisherPtr<core::AgentMetricsMsg> metrics_pub;
            // Subscribers
            core::SubscriberPtr<core::PointStampedMsg> local_position_sub;
            core::SubscriberPtr<core::PointStampedMsg> position_setpoint_sub;
            core::SubscriberPtr<core::ObservationSetpointsMsg> observation_setpoints_sub;
            // Constructor
            AgentData()
                : position(), has_position(false), setpoint(), has_setpoint(false),
                  zoom_factors(), has_observation_setpoints(false),
                  metrics_pub(), local_position_sub(), position_setpoint_sub(),
                  observation_setpoints_sub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;

    private: // Accumulated data
        AgentData agent_;
        core::PointMsg last_position_;
        float distance_traveled_;
        float total_speed_;
        int speed_samples_;
        core::Time last_update_time_;
        float time_elapsed_;
        core::Time mission_start_time_;

    private: // Callbacks
        void localPositionCallback(const core::PointStampedMsg::SharedPtr msg);
        void positionSetpointCallback(const core::PointStampedMsg::SharedPtr msg);
        void observationSetpointsCallback(const core::ObservationSetpointsMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        core::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg