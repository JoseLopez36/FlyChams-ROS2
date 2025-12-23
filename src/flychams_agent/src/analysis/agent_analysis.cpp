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
        agents_.clear();
        clusters_.clear();

        // Set update timer
        update_timer_ = rclcpp::create_timer(node_, 
            node_->get_clock(), 
            std::chrono::duration<float>(1.0f / update_rate_), 
            std::bind(&AgentAnalysis::update, this), 
            module_cb_group_);
    }

    void AgentAnalysis::onShutdown()
    {
        // Destroy agents and clusters
        agents_.clear();
        clusters_.clear();
        // Destroy update timer
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // PUBLIC METHODS: Public methods for adding/removing clusters and agents
    // ════════════════════════════════════════════════════════════════════════════

    void AgentAnalysis::addAgent(const ID& agent_id)
    {
        // Create and add agent
        agents_.insert({ agent_id, Agent() });

        // Define agent's central unit ID
        const auto& tracking_params = settings_tools_->getTrackingParameters(agent_id);
        agents_[agent_id].central_unit_id = tracking_params.observation_units_params[0].id;

        // Create agent status subscriber
        agents_[agent_id].status_sub = topic_tools_->createAgentStatusSubscriber(agent_id,
            [this, agent_id](const AgentStatusMsg::SharedPtr msg)
            {
                this->agentStatusCallback(agent_id, msg);
            }, sub_options_with_module_cb_group_);

        // Create agent assignment subscriber
        agents_[agent_id].assignment_sub = topic_tools_->createAgentAssignmentSubscriber(agent_id,
            [this, agent_id](const AgentAssignmentMsg::SharedPtr msg)
            {
                this->agentAssignmentCallback(agent_id, msg);
            }, sub_options_with_module_cb_group_);

        // Create agent clusters publisher
        agents_[agent_id].clusters_pub = topic_tools_->createAgentClustersPublisher(agent_id);
    }

    void AgentAnalysis::removeAgent(const ID& agent_id)
    {
        // Remove agent from map
        agents_.erase(agent_id);
    }

    void AgentAnalysis::addCluster(const ID& cluster_id)
    {
        // Create and add cluster
        clusters_.insert({ cluster_id, Cluster() });

        // Create cluster geometry subscriber
        clusters_[cluster_id].geometry_sub = topic_tools_->createClusterGeometrySubscriber(cluster_id,
            [this, cluster_id](const ClusterGeometryMsg::SharedPtr msg)
            {
                this->clusterGeometryCallback(cluster_id, msg);
            }, sub_options_with_module_cb_group_);
    }

    void AgentAnalysis::removeCluster(const ID& cluster_id)
    {
        // Remove cluster from map
        clusters_.erase(cluster_id);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void AgentAnalysis::agentStatusCallback(const ID& agent_id, const AgentStatusMsg::SharedPtr msg)
    {
        // Update agent status
        agents_[agent_id].status = static_cast<AgentStatus>(msg->status);
        agents_[agent_id].has_status = true;
    }

    void AgentAnalysis::agentAssignmentCallback(const ID& agent_id, const AgentAssignmentMsg::SharedPtr msg)
    {
        // Update agent assignment
        agents_[agent_id].unit_ids = msg->unit_ids;
        agents_[agent_id].cluster_ids = msg->cluster_ids;
        agents_[agent_id].has_assignment = true;
    }

    void AgentAnalysis::clusterGeometryCallback(const ID& cluster_id, const ClusterGeometryMsg::SharedPtr msg)
    {
        // Update cluster geometry
        clusters_[cluster_id].center = msg->center;
        clusters_[cluster_id].radius = msg->radius;
        clusters_[cluster_id].has_geometry = true;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE: Update analysis
    // ════════════════════════════════════════════════════════════════════════════

    void AgentAnalysis::update()
    {
        // Check if we have a valid agent status, cluster geometries and assignments
        for (const auto& [agent_id, agent] : agents_)
        {
            if (!agent.has_status || !agent.has_assignment)
            {
                RCLCPP_WARN(node_->get_logger(), "Agent analysis: Agent %s has no status or assignment", agent_id.c_str());
                return; // Skip updating if we don't have a valid agent status or assignment
            }

            // Check if we are in the correct state to analyze
            if (agent.status != AgentStatus::MISSION)
            {
                RCLCPP_WARN(node_->get_logger(), "Agent analysis: Agent %s is not in the correct state to analyze",
                    agent_id.c_str());
                return;
            }
        }
        for (const auto& [cluster_id, cluster] : clusters_)
        {
            if (!cluster.has_geometry)
            {
                RCLCPP_WARN(node_->get_logger(), "Agent analysis: Cluster %s has no geometry", cluster_id.c_str());
                return; // Skip updating if we don't have a valid cluster geometry
            }
        }

        // Create and publish clusters message for each agent
        for (const auto& [agent_id, agent] : agents_)
        {
            // Create clusters message
            AgentClustersMsg msg;
            msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());

            // Iterate over the assignment and add tracking clusters
            int n_t = static_cast<int>(agent.unit_ids.size());
            int n_o = n_t + 1;
            msg.unit_ids.resize(n_o);
            msg.centers.resize(n_o);
            msg.radii.resize(n_o);
            int c = 0;
            for (int i = 1; i < n_o; i++)
            {
                msg.unit_ids[i] = agent.unit_ids[c];
                const auto& cluster = clusters_[agent.cluster_ids[c]];
                msg.centers[i] = cluster.center;
                msg.radii[i] = cluster.radius;
                c++;
            }

            // Add central cluster
            msg.unit_ids[0] = agent.central_unit_id;
            const auto& [central_P, central_r] = computeCentralCluster(msg.centers, msg.radii);
            msg.centers[0] = central_P;
            msg.radii[0] = central_r;

            // Publish
            agent.clusters_pub->publish(msg);
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
        z_mean /= static_cast<float>(n);

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