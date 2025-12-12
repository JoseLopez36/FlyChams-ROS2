#pragma once

// Communication include
#include "flychams_control/communication/camera_communication.hpp"

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::control
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Controller for UAV mounted cameras
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-03-04
	 * ════════════════════════════════════════════════════════════════
	 */
	class CameraControl : public core::BaseModule
	{
	public: // Constructor/Destructor
		CameraControl(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr config_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
			: BaseModule(node, config_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
		{
			init();
		}

	protected: // Overrides
		void onInit() override;
		void onShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<CameraControl>;
		struct Agent
		{
			// Status data
			core::AgentStatus status;
			bool has_status;
			// Subscribers
			core::SubscriberPtr<core::AgentStatusMsg> status_sub;
			core::SubscriberPtr<core::AgentObservationSetpointsMsg> setpoints_sub;
			// Constructor
			Agent()
				: status(), has_status(false), status_sub(), setpoints_sub()
			{
			}
		};

	private: // Parameters
		core::ID agent_id_;

	private: // Data
		// Agent
		Agent agent_;
		// Camera communication
		CameraCommunication::SharedPtr camera_comm_;

	private: // Callbacks
		void statusCallback(const core::AgentStatusMsg::SharedPtr msg);
		void setpointsCallback(const core::AgentObservationSetpointsMsg::SharedPtr msg);

	private: // Camera management
		void controlCameras(const core::AgentObservationSetpointsMsg& setpoints);
	};

} // namespace flychams::control