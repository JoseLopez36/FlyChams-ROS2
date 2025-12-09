#pragma once

// Communication include
#include "flychams_control/communication/mavros_communication.hpp"

// Base module include
#include "flychams_core/base/base_module.hpp"

// Core include
#include "flychams_core/utils/geo_utils.hpp"

namespace flychams::control
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Frame manager for UAV drones
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-26
     * ════════════════════════════════════════════════════════════════
     */
    class DroneFrames : public core::BaseModule
    {
    public: // Constructor/Destructor
        DroneFrames(const core::ID& agent_id, core::NodePtr node, core::ConfigTools::SharedPtr config_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, config_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<DroneFrames>;
        struct Agent
        {
            // Global origin data
            core::GeoPointStampedMsg global_origin;
            bool has_global_origin;
            // Home position data
            core::GeoPointStampedMsg home_position;
            bool has_home_position;
            // Local odometry data
            core::OdometryMsg local_odom;
            bool has_local_odom;
            // Subscriber
            core::SubscriberPtr<core::GeoPointStampedMsg> global_origin_sub;
            core::SubscriberPtr<mavros_msgs::msg::HomePosition> home_position_sub;
            core::SubscriberPtr<core::OdometryMsg> local_odom_sub;
            // Constructor
            Agent()
                : global_origin(), has_global_origin(false), home_position(), has_home_position(false), local_odom(), has_local_odom(false),
                global_origin_sub(), home_position_sub(), local_odom_sub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;

    private: // Data
        // Agent
        Agent agent_;
        // Mavros communication
        MavrosCommunication::SharedPtr mavros_comm_;

    private: // Callbacks
        void globalOriginCallback(const core::GeoPointStampedMsg::SharedPtr msg);
        void homePositionCallback(const mavros_msgs::msg::HomePosition::SharedPtr msg);
        void localOdomCallback(const core::OdometryMsg::SharedPtr msg);

    private: // Frames creation
        void createLocalFrame();
        void createBodyFrame();
        void createCameraBodyFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr);
        void createCameraOpticalFrame(const core::ID camera_id, const core::MultiCameraConfigPtr camera_config_ptr);

    private: // Frames update
        void updateLocalFrame(const core::GeoPointMsg& home_geopoint, const core::GeoPointMsg& origin_geopoint);
        void updateBodyFrame(const core::PointMsg& position, const core::QuaternionMsg& orientation);
        void updateCameraBodyFrame(const core::ID camera_id, const core::PointMsg& position, const core::QuaternionMsg& orientation);
    };

} // namespace flychams::control