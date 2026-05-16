#include "flychams_coordinator/state/fleet_state.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void FleetState::onModuleInit()
{
    // Get parameters from parameter server
    update_rate_ = node_->getParameterOr<float>("fleet_state_rate", 1.0f);

    // Initialize data
    agents_.clear();

    // Create fleet publisher
    fleet_status_pub_ = node_->createFleetStatusPublisher();

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&FleetState::update, this));
}

void FleetState::onModuleShutdown()
{
    // Destroy update timer
    update_timer_.reset();
    // Destroy agents
    agents_.clear();
    // Destroy publishers
    fleet_status_pub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLIC METHODS: Dynamic element management
// ════════════════════════════════════════════════════════════════════════════

void FleetState::addAgent(const ID& agent_id)
{
    agents_.insert({ agent_id, Agent() });

    agents_[agent_id].status_sub = node_->createAgentStatusSubscriber(agent_id,
        [this, agent_id](const AgentStatusMsg::SharedPtr msg)
        {
            this->agentStatusCallback(agent_id, msg);
        }, node_->getSubscriptionOptions());

    RCLCPP_INFO(node_->get_logger(), "Fleet manager: Agent %s added", agent_id.c_str());
}

void FleetState::removeAgent(const ID& agent_id)
{
    agents_.erase(agent_id);
    RCLCPP_INFO(node_->get_logger(), "Fleet manager: Agent %s removed", agent_id.c_str());
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void FleetState::agentStatusCallback(const ID& agent_id, const AgentStatusMsg::SharedPtr msg)
{
    agents_[agent_id].status = static_cast<AgentStatus>(msg->status);
    agents_[agent_id].has_status = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Publish fleet status
// ════════════════════════════════════════════════════════════════════════════

void FleetState::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Fleet state: Skipping update due to invalid status");
        return;
    }

    // Compute fleet state
    FleetStatus fleet_status = computeFleetStatus();

    // Build agent id and status arrays
    std::vector<std::string> agent_ids;
    std::vector<uint8_t> agent_statuses;
    bool all_idle = true;
    bool all_active = true;
    bool any_error = false;

    for (const auto& [agent_id, agent] : agents_)
    {
        agent_ids.push_back(agent_id);
        agent_statuses.push_back(static_cast<uint8_t>(agent.status));

        if (agent.status != AgentStatus::IDLE) all_idle = false;
        if (agent.status != AgentStatus::ACTIVE) all_active = false;
        if (agent.status == AgentStatus::ERROR) any_error = true;
    }

    // Publish FleetStatus
    FleetStatusMsg fleet_msg;
    fleet_msg.header.stamp = node_->now();
    fleet_msg.status = static_cast<uint8_t>(fleet_status);
    fleet_msg.all_agents_idle = all_idle;
    fleet_msg.all_agents_active = all_active;
    fleet_msg.any_agent_error = any_error;
    fleet_msg.agent_ids = agent_ids;
    fleet_msg.agent_statuses = agent_statuses;
    fleet_status_pub_->publish(fleet_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool FleetState::checkStatus()
{
	// Check 1: All agents must have a valid status
	for (const auto& [agent_id, agent] : agents_)
	{
		if (!agent.has_status)
		{
			RCLCPP_WARN(node_->get_logger(), "Fleet state: Agent %s has no status", agent_id.c_str());
			return false;
		}
	}

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// STATE HELPERS
// ════════════════════════════════════════════════════════════════════════════

FleetStatus FleetState::computeFleetStatus() const
{
    bool all_idle = true;
    bool all_active = true;
    bool any_error = false;

    for (const auto& [agent_id, agent] : agents_)
    {
        if (agent.status != AgentStatus::IDLE) all_idle = false;
        if (agent.status != AgentStatus::ACTIVE) all_active = false;
        if (agent.status == AgentStatus::ERROR) any_error = true;
    }

    if (any_error) return FleetStatus::ERROR;
    if (all_active) return FleetStatus::ACTIVE;
    if (all_idle) return FleetStatus::IDLE;
    return FleetStatus::MIXED;
}