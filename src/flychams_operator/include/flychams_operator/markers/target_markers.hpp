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
     * @brief Per-target marker generator for Foxglove visualization
     *
     * @details
     * Subscribes to the target position topic. On each call to
     * getEntities() it appends the target cylinder and optional label
     * to the provided SceneUpdate message.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-23
     * ════════════════════════════════════════════════════════════════
     */
    class TargetMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        TargetMarkers(const common::ID& target_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), target_id_(target_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<TargetMarkers>;

    public: // Entity collection
        void getEntities(common::FoxSceneUpdateMsg& out) const;

    private: // Data
        struct TargetData
        {
            common::PointMsg position;
            bool has_position = false;
        };

    private: // Parameters
        common::ID target_id_;
        float update_rate_;

    private: // State
        TargetData target_;

    private: // Callbacks
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);

    private: // ROS components
        common::SubscriberPtr<common::PointStampedMsg> position_sub_;
    };

} // namespace flychams::operator_pkg