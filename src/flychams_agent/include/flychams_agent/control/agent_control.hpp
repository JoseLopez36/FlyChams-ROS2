#pragma once

// Utils include
#include "flychams_agent/autopilot/autopilot_communication.hpp"

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

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
	class DroneControl : public common::BaseStatusModule
	{
	public: // Constructor/Destructor
		DroneControl(const common::ID& agent_id, common::BaseStatusNode::SharedPtr node)
			: BaseStatusModule(node), agent_id_(agent_id)
		{
			init();
		}

	protected: // Overrides
		void onModuleInit() override;
		void onModuleShutdown() override;

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
			common::AgentStatus status;
			bool has_status;
			bool is_armed;
			bool is_flying;
			// Position data
			common::PointStampedMsg local_position;
			bool has_local_position;
			// Setpoint data
			common::PointStampedMsg setpoint;
			bool has_setpoint;
			// Subscribers
			common::SubscriberPtr<common::AgentStatusMsg> status_sub;
			common::SubscriberPtr<common::PointStampedMsg> local_position_sub;
			common::SubscriberPtr<common::PointStampedMsg> setpoint_sub;
			// Constructor
			Agent()
				: status(), has_status(false), is_armed(false), is_flying(false), local_position(),
				has_local_position(false), setpoint(), has_setpoint(false), status_sub(),
				local_position_sub(), setpoint_sub()
			{
			}
		};

	private: // Parameters
		common::ID agent_id_;
		float update_rate_;
		// Control mode
		ControlMode control_mode_;
		// Flight parameters
		float takeoff_altitude_;
		float mission_altitude_;
		FlyingBox flying_box_;

	private: // Data
		// Agent
		Agent agent_;
		// Command counter
		uint8_t command_counter_;
		// Last update time
		common::Time last_update_time_;
		// Arm all flag
		bool arm_all_;
		// Land all flag
		bool land_all_;
		// Return home flag
		bool return_home_;
		// PX4 communication
		AutopilotCommunication::SharedPtr autopilot_comm_;

	private: // Callbacks
		void statusCallback(const common::AgentStatusMsg::SharedPtr msg);
		void localPositionCallback(const common::PointStampedMsg::SharedPtr msg);
		void setpointPositionCallback(const common::PointStampedMsg::SharedPtr msg);
		void armAllCallback(const common::BoolMsg::SharedPtr msg);
		void landAllCallback(const common::BoolMsg::SharedPtr msg);
		void returnHomeCallback(const common::BoolMsg::SharedPtr msg);

	private: // Control management
		void update();
		bool checkStatus();

	private: // Requests (only if the agent is in the correct state)
		bool requestOffboard();
		bool requestDisarm();
		bool requestArm();
		bool requestTakeoff();
		bool requestMissionAltitude();
		bool requestHover();
		bool requestSetpoint();
		bool requestLand();

	private: // Helper methods
		bool isInsideFlyingBox(const common::PointMsg& point);

	private: // ROS components
		// Subscribers
		common::SubscriberPtr<common::BoolMsg> arm_all_sub_;
		common::SubscriberPtr<common::BoolMsg> land_all_sub_;
		common::SubscriberPtr<common::BoolMsg> return_home_sub_;
		// Timer
		common::TimerPtr update_timer_;
	};

} // namespace flychams::agent