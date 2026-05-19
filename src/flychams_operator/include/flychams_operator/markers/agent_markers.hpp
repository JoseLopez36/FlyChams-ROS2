#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Module that publishes high-quality agent scene markers
     *
     * @details
     * Publishes a foxglove_msgs/SceneUpdate with:
     *   - Solid body sphere (color-coded by status)
     *   - Semi-transparent outer glow shell
     *   - Upward orientation arrow
     *   - Text label with ID and altitude
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-18
     * ════════════════════════════════════════════════════════════════
     */
    class AgentMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        AgentMarkers(const common::ID& agent_id, const common::ID& element_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), agent_id_(agent_id), element_id_(element_id)
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
        };

    private: // Parameters
        common::ID agent_id_;
        common::ID element_id_;
        float update_rate_;

    private: // Data
        AgentData agent_;

    private: // Callbacks
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);
        void statusCallback(const common::AgentStatusMsg::SharedPtr msg);

    private: // Update
        void update();
        bool isDataValid() const;

    private: // ROS components
        common::TimerPtr update_timer_;
        common::PublisherPtr<common::FoxSceneUpdateMsg> scene_pub_;
        common::SubscriberPtr<common::PointStampedMsg> position_sub_;
        common::SubscriberPtr<common::AgentStatusMsg> status_sub_;
    };

} // namespace flychams::operator_pkg