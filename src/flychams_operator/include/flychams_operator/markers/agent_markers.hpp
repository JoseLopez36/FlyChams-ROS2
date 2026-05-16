#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

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
    class AgentMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        AgentMarkers(const common::ID& agent_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentMarkers>;
        struct AgentData
        {
            // Latest position
            common::PointMsg position;
            bool has_position;
            // Latest status
            uint8_t status;
            bool has_status;
            // Publisher
            common::PublisherPtr<common::MarkerArrayMsg> markers_pub;
            // Subscribers
            common::SubscriberPtr<common::PointStampedMsg> local_position_sub;
            common::SubscriberPtr<common::AgentStatusMsg> status_sub;
            // Constructor
            AgentData()
                : position(), has_position(false), status(0), has_status(false),
                  markers_pub(), local_position_sub(), status_sub()
            {
            }
        };

    private: // Parameters
        common::ID agent_id_;
        float update_rate_;

    private: // Data
        AgentData agent_;

    private: // Callbacks
        void localPositionCallback(const common::PointStampedMsg::SharedPtr msg);
        void statusCallback(const common::AgentStatusMsg::SharedPtr msg);

    private: // Update
        void update();
        bool checkStatus();

    private: // ROS components
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg