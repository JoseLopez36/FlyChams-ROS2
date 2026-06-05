#include "flychams_operator/metrics/fleet_metrics.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void FleetMetrics::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 1.0f);

    // Initialize data
    total_agents_ = 0;
    assignment_solve_duration_ = 0.0f;
    has_assignment_solve_duration_ = false;
    assignment_swap_count_ = 0;

    // Publishers
    metrics_pub_ = node_->createFleetMetricsPublisher();

    // Subscribers
    assignment_solve_duration_sub_ = node_->createAssignmentSolveDurationSubscriber(
        std::bind(&FleetMetrics::assignmentSolveDurationCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    assignment_swap_count_sub_ = node_->createAssignmentSwapCountSubscriber(
        std::bind(&FleetMetrics::assignmentSwapCountCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&FleetMetrics::update, this));
}

void FleetMetrics::onModuleShutdown()
{
    metrics_pub_.reset();
    assignment_solve_duration_sub_.reset();
    assignment_swap_count_sub_.reset();
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// ELEMENT MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void FleetMetrics::addAgent()
{
    total_agents_++;
}

void FleetMetrics::removeAgent()
{
    total_agents_--;
}

void FleetMetrics::assignmentSolveDurationCallback(const Float32Msg::SharedPtr msg)
{
    assignment_solve_duration_ = msg->data;
    has_assignment_solve_duration_ = true;
}

void FleetMetrics::assignmentSwapCountCallback(const Int32Msg::SharedPtr msg)
{
    // Accumulate swap count over the mission
    assignment_swap_count_ += msg->data;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void FleetMetrics::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        return;
    }

    // Build and publish message
    FleetMetricsMsg msg;
    msg.header = node_->createHeader(node_->getGlobalFrame());
    msg.total_agents = total_agents_;
    msg.assignment_swap_count = assignment_swap_count_;
    msg.assignment_solve_duration = has_assignment_solve_duration_ ? assignment_solve_duration_ : 0.0f;

    metrics_pub_->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool FleetMetrics::checkStatus()
{
    // Only publish when mission is active
    if (!node_->isMissionActive())
    {
        return false;
    }

    // All checks passed
    return true;
}