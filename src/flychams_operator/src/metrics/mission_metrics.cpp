#include "flychams_operator/metrics/mission_metrics.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void MissionMetrics::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 1.0f);

    // Initialize data
    total_agents_ = 0;
    total_targets_ = 0;
    total_clusters_ = 0;
    has_mission_started_ = false;

    // Publishers
    metrics_pub_ = node_->createMissionMetricsPublisher();

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&MissionMetrics::update, this));
}

void MissionMetrics::onModuleShutdown()
{
    metrics_pub_.reset();
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// ELEMENT MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void MissionMetrics::addAgent()
{
    total_agents_++;
}

void MissionMetrics::removeAgent()
{
    total_agents_--;
}

void MissionMetrics::addTarget()
{
    total_targets_++;
}

void MissionMetrics::removeTarget()
{
    total_targets_--;
}

void MissionMetrics::addCluster()
{
    total_clusters_++;
}

void MissionMetrics::removeCluster()
{
    total_clusters_--;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void MissionMetrics::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        return;
    }

    // Compute elapsed time
    float time_elapsed = 0.0f;
    if (has_mission_started_)
    {
        time_elapsed = static_cast<float>((node_->now() - mission_start_time_).seconds());
    }
    else if (node_->isMissionActive())
    {
        // Mission just started
        mission_start_time_ = node_->now();
        has_mission_started_ = true;
    }

    // Build and publish message
    MissionMetricsMsg msg;
    msg.header = node_->createHeader(node_->getGlobalFrame());
    msg.total_agents = total_agents_;
    msg.total_targets = total_targets_;
    msg.total_clusters = total_clusters_;
    msg.time = time_elapsed;

    metrics_pub_->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool MissionMetrics::checkStatus()
{
    // Only publish when mission is active
    if (!node_->isMissionActive())
    {
        // Reset mission started flag when mission ends
        has_mission_started_ = false;
        return false;
    }

    // All checks passed
    return true;
}