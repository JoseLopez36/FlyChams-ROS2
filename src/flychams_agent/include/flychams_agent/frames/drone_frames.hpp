#pragma once

// Utils include
#include "flychams_agent/mavros/mavros_communication.hpp"
#include "flychams_agent/mavros/mavros_utils.hpp"

// Base module include
#include "flychams_common/base/base_status_module.hpp"

// Base node include
#include "flychams_common/base/base_status_node.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Frame manager for UAV drones
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-26
     * ════════════════════════════════════════════════════════════════
     */
    class DroneFrames : public common::BaseStatusModule
    {
    public: // Constructor/Destructor
        DroneFrames(const common::ID& agent_id, common::BaseStatusNode::SharedPtr node)
            : BaseStatusModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<DroneFrames>;
        struct Agent
        {
            // Global origin data
            common::GeoPointMsg global_origin;
            bool has_global_origin;
            // Home position data
            common::GeoPointMsg home_position;
            bool has_home_position;
            // Odometry data
            common::PointMsg local_position;
            common::QuaternionMsg local_orientation;
            bool has_local_odom;
            // Subscriber
            common::SubscriberPtr<common::GeoPointStampedMsg> global_origin_sub;
            common::SubscriberPtr<mavros_msgs::msg::HomePosition> home_position_sub;
            common::SubscriberPtr<common::OdometryMsg> local_odom_sub;
            // Constructor
            Agent()
                : global_origin(), has_global_origin(false), home_position(), has_home_position(false),
                local_position(), local_orientation(), has_local_odom(false),
                global_origin_sub(), home_position_sub(), local_odom_sub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;

    private: // Data
        // Agent
        Agent agent_;
        // Mavros communication
        MavrosCommunication::SharedPtr mavros_comm_;

    private: // Callbacks
        void globalOriginCallback(const common::GeoPointStampedMsg::SharedPtr msg);
        void homePositionCallback(const mavros_msgs::msg::HomePosition::SharedPtr msg);
        void localOdomCallback(const common::OdometryMsg::SharedPtr msg);

    private: // Frames creation
        void createLocalFrame(const common::GeoPointMsg& home_geopoint, const common::GeoPointMsg& origin_geopoint);

    private: // Frames update
        void updateBodyFrame(const common::PointMsg& position, const common::QuaternionMsg& orientation);

    private: // Frames management
        void update();
        bool checkStatus();

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
    };

} // namespace flychams::agent
