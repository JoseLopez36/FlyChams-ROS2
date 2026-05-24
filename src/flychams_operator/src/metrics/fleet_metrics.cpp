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

    // Publishers
    metrics_pub_ = node_->createFleetMetricsPublisher();

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&FleetMetrics::update, this));
}

void FleetMetrics::onModuleShutdown()
{
    metrics_pub_.reset();
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
    msg.assignment_solve_duration = 0.0f;  // TODO: Not currently sourced from assignment module

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