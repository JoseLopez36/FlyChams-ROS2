#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

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
    class AgentMetrics : public common::BaseModule
    {
    public: // Constructor/Destructor
        AgentMetrics(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentMetrics>;
        struct AgentData
        {
            // Latest position
            common::PointMsg position;
            bool has_position;
            // Latest setpoint
            common::PointMsg setpoint;
            bool has_setpoint;
            // Latest zoom factors (from observation setpoints)
            std::vector<float> zoom_factors;
            bool has_observation_setpoints;
            // Publisher
            common::PublisherPtr<common::AgentMetricsMsg> metrics_pub;
            // Subscribers
            common::SubscriberPtr<common::PointStampedMsg> local_position_sub;
            common::SubscriberPtr<common::PointStampedMsg> position_setpoint_sub;
            common::SubscriberPtr<common::ObservationSetpointsMsg> observation_setpoints_sub;
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
        common::ID agent_id_;
        float update_rate_;

    private: // Accumulated data
        AgentData agent_;
        common::PointMsg last_position_;
        float distance_traveled_;
        float total_speed_;
        int speed_samples_;
        common::Time last_update_time_;
        float time_elapsed_;
        common::Time mission_start_time_;

    private: // Callbacks
        void localPositionCallback(const common::PointStampedMsg::SharedPtr msg);
        void positionSetpointCallback(const common::PointStampedMsg::SharedPtr msg);
        void observationSetpointsCallback(const common::ObservationSetpointsMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg