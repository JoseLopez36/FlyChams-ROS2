#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::coordinator
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Registration of clusters
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-03-21
	 * ════════════════════════════════════════════════════════════════
	 */
	class ClusterRegistration : public core::BaseModule
	{
	public: // Constructor/Destructor
		ClusterRegistration(core::BaseNode::SharedPtr node)
			: BaseModule(node)
		{
			init();
		}

	protected: // Overrides
		void onModuleInit() override;
		void onModuleShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<ClusterRegistration>;

	private: // Data
		core::IDs clusters_;

	public: // Methods
		const core::IDs& getClusters() const { return clusters_; }
	};

} // namespace flychams::coordinator