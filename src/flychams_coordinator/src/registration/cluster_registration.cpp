#include "flychams_coordinator/registration/cluster_registration.hpp"

using namespace flychams::common;

namespace flychams::coordinator
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void ClusterRegistration::onModuleInit()
	{
		// Iterate over all agents in the configuration
		clusters_.clear();
		int cluster_index = 0;
		for (const auto& [agent_id, agent_ptr] : node_->getSettings()->getAgentTeam())
		{
			// Get tracking parameters
			const auto& tracking_params = node_->getSettings()->getTrackingParameters(agent_id);

			// Register clusters based on the number of tracking units
			const int& n_t = tracking_params.n_t;
			for (int i = 0; i < n_t; i++)
			{
				// Generate cluster ID
				std::stringstream ss;
				ss << "CLUSTER" << std::setw(2) << std::setfill('0') << cluster_index;
				const ID cluster_id = ss.str();

				// Add clusters to list
				clusters_.push_back(cluster_id);

				// Increment cluster index
				cluster_index++;
			}
		}
	}

	void ClusterRegistration::onModuleShutdown()
	{
		// Destroy clusters
		clusters_.clear();
	}

} // namespace flychams::coordinator