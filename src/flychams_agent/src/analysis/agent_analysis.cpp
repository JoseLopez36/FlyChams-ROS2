#include "flychams_agent/analysis/agent_analysis.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void AgentAnalysis::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "analysis_rate", 20.0f);

        // Initialize data
        agent_ = Agent();
        clusters_.clear();

        // Define agent's central unit ID
        const auto& tracking_params = settings_tools_->getTrackingParameters(agent_id_);
        agent_.central_unit_id = tracking_params.observation_units_params[0].id;

        // Create agent status subscriber
        agent_.status_sub = topic_tools_->createAgentStatusSubscriber(agent_id_,
            std::bind(&AgentAnalysis::statusCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Create agent assignment subscriber
        agent_.assignment_sub = topic_tools_->createAgentAssignmentSubscriber(agent_id_,
            std::bind(&AgentAnalysis::assignmentCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Create agent clusters publisher
        agent_.clusters_pub = topic_tools_->createAgentClustersPublisher(agent_id_);

        // Set update timer
        update_timer_ = rclcpp::create_timer(node_, 
            node_->get_clock(), 
            std::chrono::duration<float>(1.0f / update_rate_), 
            std::bind(&AgentAnalysis::update, this), 
            module_cb_group_);
    }

    void AgentAnalysis::onShutdown()
    {
        // Destroy subscribers/publishers
        agent_.status_sub.reset();
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

    void AgentAnalysis::statusCallback(const AgentStatusMsg::SharedPtr msg)
    {
        // Update agent status
        agent_.status = static_cast<AgentStatus>(msg->status);
        agent_.has_status = true;
    }

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
        // Check if we have a valid agent status or assignment
        if (!agent_.has_status || !agent_.has_assignment)
        {
            // Wait for status and assignment
            return; 
        }

        // Check if we are in the correct state to analyze
        if (agent_.status != AgentStatus::MISSION)
        {
            return;
        }

        // Check if we have valid cluster geometries for assigned clusters
        for (const auto& cluster_id : agent_.cluster_ids)
        {
            if (clusters_.find(cluster_id) == clusters_.end() || !clusters_[cluster_id].has_geometry)
            {
                // Wait for cluster geometry
                return; 
            }
        }

        // Create clusters message
        AgentClustersMsg msg;
        msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());

        // Iterate over the assignment and add tracking clusters
        int n_t = static_cast<int>(agent_.unit_ids.size());
        int n_o = n_t + 1;
        msg.unit_ids.resize(n_o);
        msg.centers.resize(n_o);
        msg.radii.resize(n_o);
        
        int c = 0;
        for (int i = 1; i < n_o; i++)
        {
            msg.unit_ids[i] = agent_.unit_ids[c];
            const auto& cluster = clusters_[agent_.cluster_ids[c]];
            msg.centers[i] = cluster.center;
            msg.radii[i] = cluster.radius;
            c++;
        }

        // Add central cluster
        msg.unit_ids[0] = agent_.central_unit_id;
        const auto& [central_P, central_r] = computeCentralCluster(msg.centers, msg.radii);
        msg.centers[0] = central_P;
        msg.radii[0] = central_r;

        // Publish
        agent_.clusters_pub->publish(msg);
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
                new_cluster.geometry_sub = topic_tools_->createClusterGeometrySubscriber(id,
                    [this, id](const ClusterGeometryMsg::SharedPtr msg) {
                        this->clusterGeometryCallback(id, msg);
                    }, sub_options_with_module_cb_group_);
                clusters_[id] = std::move(new_cluster);
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // ANALYSIS: Analysis methods
    // ════════════════════════════════════════════════════════════════════════════

    std::pair<core::PointMsg, float> AgentAnalysis::computeCentralCluster(const std::vector<core::PointMsg>& centers, const std::vector<float>& radii)
    {
        // Get number of tracking units
        int n = centers.size() - 1;

        // Convert message to Eigen
        Matrix3Xr tab_P(3, n);
        RowVectorXr tab_r(n);
        int c = 0;
        for (int i = 1; i < n + 1; i++)
        {
            tab_P.col(c) = RosUtils::fromMsg(centers[i]);
            tab_r(c) = radii[i];
            c++;
        }

        // Compute mean of all available clusters
        core::Vector3r z_mean = core::Vector3r::Zero();
        for (int i = 0; i < n; i++)
        {
            z_mean += tab_P.col(i);
        }
        
        if (n > 0)
        {
            z_mean /= static_cast<float>(n);
        }

        // Get the largest possible radius
        float r_max = 0.0f;
        for (int i = 0; i < n; i++)
        {
            r_max = std::max(r_max, (z_mean - tab_P.col(i)).norm() + tab_r(i));
        }

        // Convert back to message
        PointMsg central_P;
        RosUtils::toMsg(z_mean, central_P);
        float central_r = r_max;

        // Return central cluster and radius
        return std::make_pair(central_P, central_r);
    }

} // namespace flychams::agent
