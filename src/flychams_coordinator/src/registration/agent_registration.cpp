#include "flychams_coordinator/registration/agent_registration.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void AgentRegistration::onModuleInit()
{
	// Iterate over all agents in the configuration
	agents_.clear();
	for (const auto& [agent_id, agent_ptr] : node_->getSettings()->getAgentTeam())
	{
		// Add agent to list
		agents_.push_back(agent_id);
	}
}

void AgentRegistration::onModuleShutdown()
{
	// Destroy agents
	agents_.clear();
}