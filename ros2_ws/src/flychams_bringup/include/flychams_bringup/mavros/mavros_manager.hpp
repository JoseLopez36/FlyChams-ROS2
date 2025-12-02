#pragma once

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::bringup
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief MavROS manager for managing connection to a single agent
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-12-01
	 * ════════════════════════════════════════════════════════════════
	 */
	class MavrosManager : public core::BaseModule
	{
	public: // Constructor/Destructor
		MavrosManager(const core::ID& agent_id, core::NodePtr node, core::ConfigTools::SharedPtr config_tools, core::FrameworkTools::SharedPtr framework_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
			: BaseModule(node, config_tools, framework_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
		{
			init();
		}

	protected: // Overrides
		void onInit() override;
		void onShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<MavrosManager>;

	private: // Parameters
		core::ID agent_id_;
		std::string fcu_url_;
		int tgt_system_;

	private: // Data
		pid_t mavros_pid_;

	public: // Methods
		void launchMavros();
		void shutdownMavros();
	};

} // namespace flychams::bringup