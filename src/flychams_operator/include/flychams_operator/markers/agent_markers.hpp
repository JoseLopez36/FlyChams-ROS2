#pragma once

// Utils include
#include "flychams_operator/markers/marker_parameters.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-agent marker generator for Foxglove visualization
     *
     * @details
     * Subscribes to agent position, status and setpoint topics.
     * On each call to getEntities() it appends the agent's 3D model
     * and optional setpoint marker to the provided SceneUpdate message.
     * The agent colour is derived from the mission settings idx.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-23
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

    public: // Entity collection
        void getEntities(common::FoxSceneUpdateMsg& out) const;

    private: // Data
        struct AgentData
        {
            common::PointMsg position;
            bool has_position = false;
            uint8_t status = 0;
            bool has_status = false;
            common::PointMsg setpoint;
            bool has_setpoint = false;
        };

    private: // Parameters
        common::ID agent_id_;
        int agent_idx_ = 0;
        float update_rate_;

    private: // State
        AgentData agent_;

    private: // Callbacks
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);
        void statusCallback(const common::AgentStatusMsg::SharedPtr msg);
        void setpointCallback(const common::PointStampedMsg::SharedPtr msg);

    private: // ROS components
        common::SubscriberPtr<common::PointStampedMsg> position_sub_;
        common::SubscriberPtr<common::AgentStatusMsg> status_sub_;
        common::SubscriberPtr<common::PointStampedMsg> setpoint_sub_;
    };

} // namespace flychams::operator_pkg