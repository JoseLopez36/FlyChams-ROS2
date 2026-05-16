#include "flychams_coordinator/registration/target_registration.hpp"

using namespace flychams::common;

namespace flychams::coordinator
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void TargetRegistration::onModuleInit()
	{
		// Iterate over all targets in the configuration
		targets_.clear();
		for (const auto& [target_id, target_ptr] : node_->getSettings()->getTargetGroup())
		{
			// Add target to list
			targets_.push_back(target_id);
		}
	}

	void TargetRegistration::onModuleShutdown()
	{
		// Destroy targets
		targets_.clear();
	}

} // namespace flychams::coordinator