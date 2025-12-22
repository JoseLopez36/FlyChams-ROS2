#pragma once

// Trajectory includes
#include "flychams_simulation/target/trajectory_parser.hpp"

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief State manager for targets
     *
     * @details
     * This class is responsible for managing the state of targets.
     * It handles the state of the target, mainly its instantaneus
     * position.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-27
     * ════════════════════════════════════════════════════════════════
     */
    class TargetState : public core::BaseModule
    {
    public: // Constructor/Destructor
        TargetState(const core::ID& target_id, core::NodePtr node, core::SettingsTools::SharedPtr settings_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, settings_tools, topic_tools, transform_tools, module_cb_group), target_id_(target_id)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<TargetState>;
        struct Trajectory
        {
            std::vector<TrajectoryParser::Point> points;
            int current_idx;
            int num_points;
            bool reverse;
            // Constructor
            Trajectory() : points(), current_idx(0), num_points(0), reverse(false) {}
        };
        struct Target
        {
            // Position message
            core::PointStampedMsg position;
            // Publisher
            core::PublisherPtr<core::PointStampedMsg> position_pub;
            // Constructor
            Target()
                : position(), position_pub()
            {
            }
        };

    private: // Parameters
        core::ID target_id_;
        float update_rate_;
        float cmd_timeout_;

    private: // Data
        // Trajectory
        Trajectory trajectory_;
        // Target
        Target target_;
        // Time step
        float time_elapsed_;
        core::Time last_update_time_;

    private: // State management
        void update();

    private:
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::simulation