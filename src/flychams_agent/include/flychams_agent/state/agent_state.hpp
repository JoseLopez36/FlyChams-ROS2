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
     * @brief State manager for UAV drones
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-26
     * ════════════════════════════════════════════════════════════════
     */
    class DroneState : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        DroneState(const common::ID& agent_id, common::BaseStatusNode::SharedPtr node)
            : BaseStatusModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<DroneState>;
        struct Agent
        {
            // State data
            px4_msgs::msg::VehicleStatus vehicle_status;
            bool has_status;
            // Odometry data
            px4_msgs::msg::VehicleOdometry vehicle_odom;
            bool has_local_odom;
            // Subscriber
            common::SubscriberPtr<px4_msgs::msg::VehicleStatus> status_sub;
            common::SubscriberPtr<px4_msgs::msg::VehicleOdometry> local_odom_sub;
            // Publishers
            common::PublisherPtr<common::AgentStatusMsg> status_pub;
            common::PublisherPtr<common::PointStampedMsg> local_position_pub;
            common::PublisherPtr<common::PointStampedMsg> global_position_pub;
            // Constructor
            Agent()
                : vehicle_status(), has_status(false), vehicle_odom(), has_local_odom(false),
                status_sub(), local_odom_sub(),
                status_pub(), local_position_pub(), global_position_pub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;
        // Flight parameters
        float takeoff_altitude_;
        float mission_altitude_;
        float landing_altitude_;

    private: // Data
        // Agent
        Agent agent_;
        // PX4 communication
        AutopilotCommunication::SharedPtr autopilot_comm_;

    private: // Callbacks
        void vehicleStatusCallback(const px4_msgs::msg::VehicleStatus::SharedPtr msg);
        void localOdomCallback(const px4_msgs::msg::VehicleOdometry::SharedPtr msg);

    private: // Status management
        void update();
        bool checkStatus();
        
    private: // Status update methods
        void updateLocalPosition(const px4_msgs::msg::VehicleOdometry& vehicle_odom);
        void updateGlobalPosition(const px4_msgs::msg::VehicleOdometry& vehicle_odom);

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::agent