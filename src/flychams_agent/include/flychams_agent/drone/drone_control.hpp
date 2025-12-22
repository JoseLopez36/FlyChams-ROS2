#pragma once

// Communication include
#include "flychams_agent/mavros/mavros_communication.hpp"

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::agent
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Motion manager for UAV drones
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-01-29
	 * ════════════════════════════════════════════════════════════════
	 */
	class DroneControl : public core::BaseModule
	{
	public: // Constructor/Destructor
		DroneControl(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
			: BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
		{
			init();
		}

	protected: // Overrides
		void onInit() override;
		void onShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<DroneControl>;
		enum class ControlMode
		{
			POSITION,
			VELOCITY
		};
		struct FlyingBox
		{
			float min_x, max_x;
			float min_y, max_y;
			float min_z, max_z;
		};
		struct Agent
		{
			// Status data
			core::AgentStatus status;
			bool has_status;
			// Position data
			core::PointStampedMsg local_position;
			bool has_local_position;
			// Setpoint data
			core::PointStampedMsg setpoint;
			bool has_setpoint;
			// Subscribers
			core::SubscriberPtr<core::AgentStatusMsg> status_sub;
			core::SubscriberPtr<core::PointStampedMsg> local_position_sub;
			core::SubscriberPtr<core::PointStampedMsg> setpoint_sub;
			// Constructor
			Agent()
				: status(), has_status(false), local_position(), has_local_position(false), setpoint(),
				has_setpoint(false), status_sub(), local_position_sub(), setpoint_sub()
			{
			}
		};

	private: // Parameters
		core::ID agent_id_;
		float update_rate_;
		// Control mode
		ControlMode control_mode_;
		// Flight parameters
		float takeoff_altitude_;
		FlyingBox flying_box_;

	private: // Data
		// Agent
		Agent agent_;
		// Command counter
		uint8_t command_counter_;
		// Last update time
		core::Time last_update_time_;
		// Mavros communication
		MavrosCommunication::SharedPtr mavros_comm_;

	private: // Callbacks
		void statusCallback(const core::AgentStatusMsg::SharedPtr msg);
		void localPositionCallback(const core::PointStampedMsg::SharedPtr msg);
		void setpointPositionCallback(const core::PointStampedMsg::SharedPtr msg);

	private: // Control management
		void update();

	private: // Requests (only if the agent is in the correct state)
		bool requestOffboard();
		bool requestDisarm();
		bool requestArm();
		bool requestTakeoff();
		bool requestHover();
		bool requestSetpoint();
		bool requestLand();

	private: // Helper methods
		bool isInsideFlyingBox(const core::PointMsg& point);

	private: // ROS components
		// Timer
		core::TimerPtr update_timer_;
	};

} // namespace flychams::agent