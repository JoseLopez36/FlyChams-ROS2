#pragma once

// Settings includes
#include "flychams_common/settings/mission_settings_parser.hpp"

// Types includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/config_types.hpp"
#include "flychams_common/types/ros_types.hpp"

// Utils includes
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Config Manager for getting configuration parameters from the
     * ROS2 parameters server.
     *
     * @details
     * This class provides utilities for getting configuration parameters
     * from the ROS2 parameters server.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class SettingsTools
    {
    public: // Constructor/Destructor
        SettingsTools(NodePtr node);

        ~SettingsTools()
        {
            shutdown();
        }

        void shutdown()
        {
            // Destroy configuration pointer
            config_ptr_.reset();
            // Destroy node
            node_.reset();
        }

    public: // Types
        using SharedPtr = std::shared_ptr<SettingsTools>;

    private: // Data
        // Configuration
        MissionConfigPtr config_ptr_;

    public: // Raw getter methods

        const MissionConfigPtr getConfig() const
        {
            return config_ptr_;
        }

        const EnvironmentConfig getEnvironment() const
        {
            return config_ptr_->environment;
        }

        const TargetGroupConfig getTargetGroup() const
        {
            return config_ptr_->target_group;
        }

        const TargetConfigPtr getTarget(const ID& target_id) const
        {
            return config_ptr_->target_group.at(target_id);
        }

        const AgentTeamConfig getAgentTeam() const
        {
            return config_ptr_->agent_team;
        }

        const AgentConfigPtr getAgent(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id);
        }

        const TrackingConfig getTracking(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking;
        }

        const MultiCameraSetConfig getMultiCameraSet(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set;
        }

        const MultiCameraConfigPtr getMultiCamera(const ID& agent_id, const ID& multi_camera_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set.at(multi_camera_id);
        }

        const MultiWindowSetConfig getMultiWindowSet(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_window_set;
        }

        const MultiWindowConfigPtr getMultiWindow(const ID& agent_id, const ID& multi_window_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_window_set.at(multi_window_id);
        }

        const DroneConfig getDrone(const ID& agent_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->drone;
        }

        const CameraConfig getCamera(const ID& agent_id, const ID& multi_camera_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set.at(multi_camera_id)->camera;
        }

        const GimbalConfig getGimbal(const ID& agent_id, const ID& multi_camera_id) const
        {
            return config_ptr_->agent_team.at(agent_id)->tracking.multi_camera_set.at(multi_camera_id)->gimbal;
        }

        const SystemParameters getSystem() const
        {
            return config_ptr_->system;
        }

        const TopicParameters getTopics() const
        {
            return config_ptr_->topics;
        }

        const FrameParameters getFrames() const
        {
            return config_ptr_->frames;
        }

    public: // Processing getter methods
        const TrackingParameters getTrackingParameters(const std::string& agent_id) const;
        const ObservationUnitParameters getObservationUnitParameters(const TrackingConfig& tracking, const MultiCameraConfigPtr& multi_camera) const;
        const ObservationUnitParameters getObservationUnitParameters(const TrackingConfig& tracking, const ObservationUnitParameters& central_camera_params, const MultiWindowConfigPtr& multi_window) const;

    public: // Utility methods
        void printSettings() const;

    private: // ROS components
        // Node
        NodePtr node_;
    };

} // namespace flychams::common