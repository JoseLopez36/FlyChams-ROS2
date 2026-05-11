#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::coordinator
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Registration of UAV agents
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-03-21
	 * ════════════════════════════════════════════════════════════════
	 */
	class AgentRegistration : public core::BaseModule
	{
	public: // Constructor/Destructor
		AgentRegistration(core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
			: BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group)
		{
			init();
		}

	protected: // Overrides
		void onInit() override;
		void onShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<AgentRegistration>;

	private: // Data
		core::IDs agents_;

	public: // Methods
		const core::IDs& getAgents() const { return agents_; }
	};

} // namespace flychams::coordinator