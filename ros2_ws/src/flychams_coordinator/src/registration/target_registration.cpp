#include "flychams_coordinator/registration/target_registration.hpp"

using namespace flychams::core;

namespace flychams::coordinator
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void TargetRegistration::onInit()
	{
		// Iterate over all targets in the configuration
		targets_.clear();
		for (const auto& [target_id, target_ptr] : settings_tools_->getTargetGroup())
		{
			// Add target to list
			targets_.push_back(target_id);
		}
	}

	void TargetRegistration::onShutdown()
	{
		// Destroy targets
		targets_.clear();
	}

} // namespace flychams::coordinator