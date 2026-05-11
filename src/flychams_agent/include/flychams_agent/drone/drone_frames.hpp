#pragma once

// Communication include
#include "flychams_agent/mavros/mavros_communication.hpp"

// Mavros include
#include "flychams_agent/mavros/mavros_utils.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

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
    class DroneFrames : public core::BaseModule
    {
    public: // Constructor/Destructor
        DroneFrames(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
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
            core::GeoPointMsg global_origin;
            bool has_global_origin;
            // Home position data
            core::GeoPointMsg home_position;
            bool has_home_position;
            // Subscriber
            core::SubscriberPtr<core::GeoPointStampedMsg> global_origin_sub;
            core::SubscriberPtr<mavros_msgs::msg::HomePosition> home_position_sub;
            core::SubscriberPtr<core::OdometryMsg> local_odom_sub;
            // Constructor
            Agent()
                : global_origin(), has_global_origin(false), home_position(), has_home_position(false),
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
        void createLocalFrame(const core::GeoPointMsg& home_geopoint, const core::GeoPointMsg& origin_geopoint);

    private: // Frames update
        void updateBodyFrame(const core::PointMsg& position, const core::QuaternionMsg& orientation);
    };

} // namespace flychams::agent
