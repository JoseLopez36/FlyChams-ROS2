#include "flychams_operator/metrics/agent_metrics.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void AgentMetrics::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Initialize data
    agent_ = AgentData();
    distance_traveled_ = 0.0f;
    last_update_time_ = node_->now();

    // Publishers
    agent_.metrics_pub = node_->createAgentMetricsPublisher(agent_id_);

    // Subscribers
    agent_.local_position_sub = node_->createAgentLocalPositionSubscriber(agent_id_,
        std::bind(&AgentMetrics::localPositionCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    agent_.position_setpoint_sub = node_->createAgentPositionSetpointSubscriber(agent_id_,
        std::bind(&AgentMetrics::positionSetpointCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    agent_.observation_setpoints_sub = node_->createObservationSetpointsSubscriber(agent_id_,
        std::bind(&AgentMetrics::observationSetpointsCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentMetrics::update, this));
}

void AgentMetrics::onModuleShutdown()
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
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Agent metrics: Skipping update due to invalid status");
        return;
    }

    // Compute dt
    auto now = node_->now();
    float dt = static_cast<float>((now - last_update_time_).seconds());
    last_update_time_ = now;

    // Compute instantaneous speed
    float dx = agent_.position.x - last_position_.x;
    float dy = agent_.position.y - last_position_.y;
    float dz = agent_.position.z - last_position_.z;
    float speed = (dt > 0.0f) ? std::sqrt(dx * dx + dy * dy + dz * dz) / dt : 0.0f;
    last_position_ = agent_.position;

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
    msg.header = node_->createHeader(node_->getGlobalFrame());
    msg.position = agent_.position;
    msg.setpoint = agent_.has_setpoint ? agent_.setpoint : agent_.position;
    msg.zoom_factors = agent_.has_observation_setpoints ? agent_.zoom_factors : std::vector<float>{};
    msg.position_solve_duration = 0.0f;  // TODO: Not currently sourced from positioning module
    msg.distance_traveled = distance_traveled_;
    msg.speed = speed;
    msg.distance_to_goal = distance_to_goal;

    agent_.metrics_pub->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool AgentMetrics::checkStatus()
{
    // Check 1: Mission must be active
    if (!node_->isMissionActive())
    {
        RCLCPP_INFO(node_->get_logger(), "Target clustering: Mission is not active");
        return false;
    }

    // Check 2: Agent must have a valid position
    if (!agent_.has_position)
    {
        RCLCPP_INFO(node_->get_logger(), "Agent metrics: Agent %s has no position", agent_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}