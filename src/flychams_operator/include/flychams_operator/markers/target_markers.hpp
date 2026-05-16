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
     * Subscribes to a target's position topic. On each timer tick it
     * publishes a MarkerArray containing a CYLINDER marker representing
     * the target body and a TEXT marker with its ID.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-14
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
        struct TargetData
        {
            // Latest position
            common::PointMsg position;
            bool has_position;
            // Publisher
            common::PublisherPtr<common::MarkerArrayMsg> markers_pub;
            // Subscribers
            common::SubscriberPtr<common::PointStampedMsg> position_sub;
            // Constructor
            TargetData()
                : position(), has_position(false), markers_pub(), position_sub()
            {
            }
        };

    private: // Parameters
        common::ID target_id_;
        float update_rate_;

    private: // Data
        TargetData target_;

    private: // Callbacks
        void positionCallback(const common::PointStampedMsg::SharedPtr msg);

    private: // Update
        void update();
        bool checkStatus();

    private: // ROS components
        common::TimerPtr update_timer_;
    };

} // namespace flychams::operator_pkg