#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::coordinator
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Registration of tracking targets
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-03-21
	 * ════════════════════════════════════════════════════════════════
	 */
	class TargetRegistration : public common::BaseModule
	{
	public: // Constructor/Destructor
		TargetRegistration(common::BaseNode::SharedPtr node)
			: BaseModule(node)
		{
			init();
		}

	protected: // Overrides
		void onModuleInit() override;
		void onModuleShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<TargetRegistration>;

	private: // Data
		common::IDs targets_;

	public: // Methods
		const common::IDs& getTargets() const { return targets_; }
	};

} // namespace flychams::coordinator