#pragma once

// Utils include
#include "flychams_coordinator/registration/agent_registration.hpp"
#include "flychams_coordinator/registration/target_registration.hpp"
#include "flychams_coordinator/registration/cluster_registration.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::coordinator
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Element registration system
	 *
	 * @details
	 * This class is responsible for registering all elements (agents, targets,
	 * clusters) in the simulation. It validates that all required elements are
	 * registered, maintains a registry of all elements, and publishes registration
	 * updates periodically. It also publishes the global origin geopoint.
	 *
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-03-21
	 * ════════════════════════════════════════════════════════════════
	 */
	class ElementRegistration : public core::BaseModule
	{
	public: // Constructor/Destructor
		ElementRegistration(core::BaseNode::SharedPtr node)
			: BaseModule(node)
		{
			init();
		}

	protected: // Overrides
		void onModuleInit() override;
		void onModuleShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<ElementRegistration>;

	private: // Registration instances
		AgentRegistration::SharedPtr agent_registration_;
		TargetRegistration::SharedPtr target_registration_;
		ClusterRegistration::SharedPtr cluster_registration_;

	private: // Elements
		core::IDs agents_;
		core::IDs targets_;
		core::IDs clusters_;

	private: // Registered elements map
		std::unordered_map<core::ID, core::ElementType> registered_elements_;

	private: // Registration methods
		void registerElement(const core::ID& element_id, const core::ElementType& element_type);
		void unregisterElement(const core::ID& element_id, const core::ElementType& element_type);
		void publishRegistration();

	private: // ROS components
		// Timer
		core::TimerPtr update_timer_;
		// Publishers
		core::PublisherPtr<core::RegistrationMsg> registration_pub_;
		core::PublisherPtr<core::GeoPointStampedMsg> global_origin_pub_;
	};

} // namespace flychams::coordinator