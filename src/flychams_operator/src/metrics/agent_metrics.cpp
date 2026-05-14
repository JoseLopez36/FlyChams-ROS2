#include "flychams_operator/metrics/agent_metrics.hpp"

#include "flychams_common/utils/ros_utils.hpp"
#include "flychams_common/utils/math_utils.hpp"

using namespace flychams::core;

namespace flychams::operator_pkg
{
    // ════════════════════════════════════════════════════════════════════════════
    // INIT / SHUTDOWN
    // ════════════════════════════════════════════════════════════════════════════

    void AgentMetrics::onInit()
    {
        // Get parameters
        update_rate_ = RosUtils::getParameterOr<float>(node_, "update_rate", 10.0f);

        // Initialize data
        agent_ = AgentData();
        distance_traveled_ = 0.0f;
        total_speed_ = 0.0f;
        speed_samples_ = 0;
        time_elapsed_ = 0.0f;
        last_update_time_ = RosUtils::now(node_);
        mission_start_time_ = RosUtils::now(node_);

        // Publishers
        agent_.metrics_pub = topic_tools_->createAgentMetricsPublisher(agent_id_);

        // Subscribers
        agent_.local_position_sub = topic_tools_->createAgentLocalPositionSubscriber(agent_id_,
            std::bind(&AgentMetrics::localPositionCallback, this, std::placeholders::_1),
            sub_options_with_module_cb_group_);

        agent_.position_setpoint_sub = topic_tools_->createAgentPositionSetpointSubscriber(agent_id_,
            std::bind(&AgentMetrics::positionSetpointCallback, this, std::placeholders::_1),
            sub_options_with_module_cb_group_);

        agent_.observation_setpoints_sub = topic_tools_->createObservationSetpointsSubscriber(agent_id_,
            std::bind(&AgentMetrics::observationSetpointsCallback, this, std::placeholders::_1),
            sub_options_with_module_cb_group_);

        // Update timer
        update_timer_ = rclcpp::create_timer(node_,
            node_->get_clock(),
            std::chrono::duration<float>(1.0f / update_rate_),
            std::bind(&AgentMetrics::update, this),
            module_cb_group_);
    }

    void AgentMetrics::onShutdown()
    {
        agent_.metrics_pub.reset();
        agent_.local_position_sub.reset();
        agent_.position_setpoint_sub.reset();
        agent_.observation_setpoints_sub.reset();
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS
    // ════════════════════════════════════════════════════════════════════════════

    void AgentMetrics::localPositionCallback(const PointStampedMsg::SharedPtr msg)
    {
        if (agent_.has_position)
        {
            // Accumulate traveled distance
            float dx = msg->point.x - agent_.position.x;
            float dy = msg->point.y - agent_.position.y;
            float dz = msg->point.z - agent_.position.z;
            distance_traveled_ += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        else
        {
            last_position_ = msg->point;
        }
        agent_.position = msg->point;
        agent_.has_position = true;
    }

    void AgentMetrics::positionSetpointCallback(const PointStampedMsg::SharedPtr msg)
    {
        agent_.setpoint = msg->point;
        agent_.has_setpoint = true;
    }

    void AgentMetrics::observationSetpointsCallback(const ObservationSetpointsMsg::SharedPtr msg)
    {
        agent_.zoom_factors.assign(msg->zoom_factors.begin(), msg->zoom_factors.end());
        agent_.has_observation_setpoints = true;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE
    // ════════════════════════════════════════════════════════════════════════════

    void AgentMetrics::update()
    {
        if (!agent_.has_position)
        {
            return;
        }

        // Compute dt
        auto now = RosUtils::now(node_);
        float dt = static_cast<float>((now - last_update_time_).seconds());
        last_update_time_ = now;
        time_elapsed_ = static_cast<float>((now - mission_start_time_).seconds());

        // Compute instantaneous speed
        float dx = agent_.position.x - last_position_.x;
        float dy = agent_.position.y - last_position_.y;
        float dz = agent_.position.z - last_position_.z;
        float speed = (dt > 0.0f) ? std::sqrt(dx * dx + dy * dy + dz * dz) / dt : 0.0f;
        last_position_ = agent_.position;

        // Accumulate speed for average
        total_speed_ += speed;
        speed_samples_++;
        float average_speed = (speed_samples_ > 0) ? total_speed_ / static_cast<float>(speed_samples_) : 0.0f;

        // Compute distance to goal
        float distance_to_goal = 0.0f;
        if (agent_.has_setpoint)
        {
            float gx = agent_.setpoint.x - agent_.position.x;
            float gy = agent_.setpoint.y - agent_.position.y;
            float gz = agent_.setpoint.z - agent_.position.z;
            distance_to_goal = std::sqrt(gx * gx + gy * gy + gz * gz);
        }

        // Build and publish message
        AgentMetricsMsg msg;
        msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        msg.position = agent_.position;
        msg.setpoint = agent_.has_setpoint ? agent_.setpoint : agent_.position;
        msg.zoom_factors = agent_.has_observation_setpoints ? agent_.zoom_factors : std::vector<float>{};
        msg.optimization_duration = 0.0f;
        msg.distance_traveled = distance_traveled_;
        msg.speed = speed;
        msg.distance_to_goal = distance_to_goal;
        msg.time_elapsed = time_elapsed_;
        msg.average_speed = average_speed;

        agent_.metrics_pub->publish(msg);
    }

} // namespace flychams::operator_pkg