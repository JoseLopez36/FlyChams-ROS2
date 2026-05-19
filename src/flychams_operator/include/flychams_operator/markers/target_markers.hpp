#pragma once

// Base module include
#include "flychams_common/base/base_module.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::operator_pkg
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Per-target marker publisher for Foxglove visualization
     *
     * @details
     * Publishes a foxglove_msgs/SceneUpdate with:
     *   - Cylinder body (human silhouette)
     *   - Semi-transparent glow shell
     *   - Ground-plane detection ring
     *   - Text label with ID
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-18
     * ════════════════════════════════════════════════════════════════
     */
    class TargetMarkers : public common::BaseModule
    {
    public: // Constructor/Destructor
        TargetMarkers(const common::ID& target_id, const common::ID& element_id, common::BaseNode::SharedPtr node)
            : BaseModule(node), target_id_(target_id), element_id_(element_id)
        {
            init();
        }

    protected: // Overrides
        void onModuleInit() override;
        void onModuleShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<TargetMarkers>;
        struct TargetData
        {
            common::PointMsg position;
            bool has_position = false;
        };

    private: // Parameters
        common::ID target_id_;
        common::ID element_id_;
        float update_rate_;

    private: // Data
        TargetData target_;

    private: // Callbacks
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);

    private: // Update methods
        void update();

    private: // Status check methods
        bool isDataValid() const;

    private: // ROS components
        // Timer
        common::TimerPtr update_timer_;
        // Publishers
        common::PublisherPtr<common::FoxSceneUpdateMsg> scene_pub_;
        // Subscribers
        common::SubscriberPtr<common::PointStampedMsg> position_sub_;
    };

} // namespace flychams::operator_pkg