#pragma once

// Module includes
#include "flychams_operator/markers/agent_markers.hpp"
#include "flychams_operator/markers/target_markers.hpp"
#include "flychams_operator/markers/cluster_markers.hpp"

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

// Standard includes
#include <unordered_map>

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Orchestrates all element-specific marker generators and
     *        publishes the full scene in a single SceneUpdate message
     *
     * @details
     * Owns one AgentMarkers and one ClusterMarkers per discovered agent,
     * and one TargetMarkers per discovered target. On each timer tick it
     * collects entities from every sub-generator and publishes them
     * together on the shared scene topic.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-23
     * ════════════════════════════════════════════════════════════════
     */
    class MarkersGenerator : public common::BaseModule
    {
    public: // Constructor/Destructor
        explicit MarkersGenerator(common::BaseNode::SharedPtr node)
            : BaseModule(node)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<MarkersGenerator>;

    public: // Element management
        void addAgent(const common::ID& agent_id);
        void removeAgent(const common::ID& agent_id);

        void addTarget(const common::ID& target_id);
        void removeTarget(const common::ID& target_id);

    private: // Update
        void update();

    private: // Parameters
        float update_rate_;

    private: // Sub-generators
        std::unordered_map<common::ID, AgentMarkers::SharedPtr>   agent_markers_;
        std::unordered_map<common::ID, TargetMarkers::SharedPtr>  target_markers_;
        std::unordered_map<common::ID, ClusterMarkers::SharedPtr> cluster_markers_;

    private: // ROS components
        common::TimerPtr update_timer_;
        common::PublisherPtr<common::FoxSceneUpdateMsg> scene_pub_;
    };

} // namespace flychams::operator_pkg