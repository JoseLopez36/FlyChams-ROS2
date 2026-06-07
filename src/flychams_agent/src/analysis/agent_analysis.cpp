#include "flychams_agent/analysis/agent_analysis.hpp"

using namespace flychams::common;

using namespace flychams::agent;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void AgentAnalysis::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate
    update_rate_ = node_->getParameterOr<float>("analysis_rate", 20.0f);

    // Initialize data
    agent_ = Agent();
    clusters_.clear();

    // Create agent assignment subscriber
    agent_.assignment_sub = node_->createAgentAssignmentSubscriber(agent_id_,
        std::bind(&AgentAnalysis::assignmentCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Create agent clusters publisher
    agent_.clusters_pub = node_->createAgentClustersPublisher(agent_id_);

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentAnalysis::update, this));
}

void AgentAnalysis::onModuleShutdown()
{
    // Destroy subscribers/publishers
    agent_.assignment_sub.reset();
    agent_.clusters_pub.reset();
    
    // Destroy clusters
    clusters_.clear();
    
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void AgentAnalysis::assignmentCallback(const AgentAssignmentMsg::SharedPtr msg)
{
    // Update agent assignment
    agent_.unit_ids = msg->unit_ids;
    agent_.cluster_ids = msg->cluster_ids;
    agent_.has_assignment = true;

    // Update cluster subscriptions
    updateClusterSubscriptions(agent_.cluster_ids);
}

void AgentAnalysis::clusterGeometryCallback(const ID& cluster_id, const ClusterGeometryMsg::SharedPtr msg)
{
    // Update cluster geometry
    if (clusters_.find(cluster_id) != clusters_.end())
    {
        clusters_[cluster_id].center = msg->center;
        clusters_[cluster_id].radius = msg->radius;
        clusters_[cluster_id].has_geometry = true;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update analysis
// ════════════════════════════════════════════════════════════════════════════

void AgentAnalysis::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Agent analysis: Skipping update due to invalid status");
        return;
    }

    // Create clusters message
    AgentClustersMsg msg;
    msg.header = node_->createHeader(node_->getGlobalFrame());

    // Iterate over the assignment and add tracking clusters
    const int n_t = static_cast<int>(agent_.unit_ids.size());
    msg.unit_ids.resize(n_t);
    msg.centers.resize(n_t);
    msg.radii.resize(n_t);

    for (int i = 0; i < n_t; i++)
    {
        msg.unit_ids[i] = agent_.unit_ids[i];
        const auto& cluster = clusters_[agent_.cluster_ids[i]];
        msg.centers[i] = cluster.center;
        msg.radii[i] = cluster.radius;
    }

    // Publish
    agent_.clusters_pub->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool AgentAnalysis::checkStatus()
{
    // Check 1: Mission must be active
    if (!node_->isMissionActive())
    {
        RCLCPP_INFO(node_->get_logger(), "Agent analysis: Mission is not active");
        return false;
    }

    // Check 2: Agent must have a valid assignment
    if (!agent_.has_assignment)
    {
        RCLCPP_INFO(node_->get_logger(), "Agent analysis: Agent %s has no assignment", agent_id_.c_str());
        return false;
    }

    // Check 3: All assigned clusters must have valid geometries
    for (const auto& cluster_id : agent_.cluster_ids)
    {
        if (clusters_.find(cluster_id) == clusters_.end() || !clusters_[cluster_id].has_geometry)
        {
            RCLCPP_INFO(node_->get_logger(), "Agent analysis: Cluster %s has no geometry", cluster_id.c_str());
            return false;
        }
    }

    // All checks passed
    return true;
}

void AgentAnalysis::updateClusterSubscriptions(const std::vector<ID>& new_cluster_ids)
{
    // Identify clusters to remove
    std::vector<ID> to_remove;
    for (const auto& [id, cluster] : clusters_)
    {
        if (std::find(new_cluster_ids.begin(), new_cluster_ids.end(), id) == new_cluster_ids.end())
        {
            to_remove.push_back(id);
        }
    }
    
    // Remove old clusters
    for (const auto& id : to_remove)
    {
        clusters_.erase(id);
    }

    // Identify clusters to add
    for (const auto& id : new_cluster_ids)
    {
        if (clusters_.find(id) == clusters_.end())
        {
            Cluster new_cluster;
            new_cluster.geometry_sub = node_->createClusterGeometrySubscriber(id,
                [this, id](const ClusterGeometryMsg::SharedPtr msg) {
                    this->clusterGeometryCallback(id, msg);
                }, node_->getSubscriptionOptions());
            clusters_[id] = std::move(new_cluster);
        }
    }
}