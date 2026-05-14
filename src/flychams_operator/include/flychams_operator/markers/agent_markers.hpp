#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-agent marker publisher for Foxglove visualization
     *
     * @details
     * Subscribes to an agent's local position and status topics.
     * On each timer tick it publishes a MarkerArray containing an
     * ARROW marker representing the agent's drone body and a TEXT
     * marker with its ID and status.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
     * ════════════════════════════════════════════════════════════════
     */
    class AgentMarkers : public core::BaseModule
    {
    public: // Constructor/Destructor
        AgentMarkers(const core::ID& agent_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentMarkers>;
        struct AgentData
        {
            // Latest position
            core::PointMsg position;
            bool has_position;
            // Latest status
            uint8_t status;
            bool has_status;
            // Publisher
            core::PublisherPtr<core::MarkerArrayMsg> markers_pub;
            // Subscribers
            core::SubscriberPtr<core::PointStampedMsg> local_position_sub;
            core::SubscriberPtr<core::AgentStatusMsg> status_sub;
            // Constructor
            AgentData()
                : position(), has_position(false), status(0), has_status(false),
                  markers_pub(), local_position_sub(), status_sub()
            {
            }
        };

    private: // Parameters
        core::ID agent_id_;
        float update_rate_;

    private: // Data
        AgentData agent_;

    private: // Callbacks
        void localPositionCallback(const core::PointStampedMsg::SharedPtr msg);
        void statusCallback(const core::AgentStatusMsg::SharedPtr msg);

    private: // Update
        void update();

    private: // ROS components
        core::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg