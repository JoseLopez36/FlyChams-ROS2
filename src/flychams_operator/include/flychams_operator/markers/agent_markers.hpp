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
     * @brief Module that publishes agent scene markers
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-18
     * ════════════════════════════════════════════════════════════════
     */
    class AgentMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        AgentMarkers(const common::ID& agent_id, int agent_idx, const common::ID& element_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id), agent_idx_(agent_idx), element_id_(element_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<AgentMarkers>;

    private: // Agent data
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
        int agent_idx_;
        common::ID element_id_;
        float update_rate_;

    private: // Data
        AgentData agent_;

    private: // Callbacks
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);
        void statusCallback(const common::AgentStatusMsg::SharedPtr msg);
        void setpointCallback(const common::PointStampedMsg::SharedPtr msg);

    private: // Update
        void update();
        bool isDataValid() const;

    private: // ROS components
        common::TimerPtr update_timer_;
        common::PublisherPtr<common::FoxSceneUpdateMsg> scene_pub_;
        common::SubscriberPtr<common::PointStampedMsg> position_sub_;
        common::SubscriberPtr<common::AgentStatusMsg> status_sub_;
        common::SubscriberPtr<common::PointStampedMsg> setpoint_sub_;
    };

} // namespace flychams::operator_pkg