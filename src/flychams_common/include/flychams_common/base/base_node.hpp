#pragma once

// Settings include
#include "flychams_common/settings/settings_tools.hpp"

// TF2 includes
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

// Types includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/config_types.hpp"
#include "flychams_common/types/ros_types.hpp"

// Utils includes
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"
#include "flychams_common/utils/ros_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base node with common functionality and tools
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class BaseNode : public rclcpp::Node
    {
    public: // Constructor/Destructor
        BaseNode(const std::string& node_name, const rclcpp::NodeOptions& options);

        void init();

        virtual ~BaseNode();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseNode>;
        // Topic structures
        struct CoordinatorTopics
        {
            std::string registration;
            std::string fleet_status;
            std::string mission_status;
            std::string global_origin;
            std::string target_position_pattern;
            std::string cluster_assignment_pattern;
            std::string cluster_geometry_pattern;
        };
        struct AgentTopics
        {
            std::string status_pattern;
            std::string global_position_pattern;
            std::string local_position_pattern;
            std::string assignment_pattern;
            std::string clusters_pattern;
            std::string position_setpoint_pattern;
            std::string observation_setpoints_pattern;
            std::string multi_camera_image_pattern;
            std::string multi_window_image_pattern;
        };
        struct OperatorTopics
        {
            std::string metrics;
            std::string agent_metrics_pattern;
            std::string target_metrics_pattern;
            std::string cluster_metrics_pattern;
            std::string agent_markers_pattern;
            std::string target_markers_pattern;
            std::string cluster_markers_pattern;
        };
        // Frame structures
        struct AgentFrames
        {
            std::string agent_local_pattern_;
            std::string agent_body_pattern_;
            std::string camera_body_pattern_;
            std::string camera_optical_pattern_;
        };

    protected: // Overridable methods
        virtual void onNodeInit() {}
        virtual void onNodeShutdown() {}

    private: // Settings data
        SettingsTools::SharedPtr settings_;

    private: // Topic data
        CoordinatorTopics coordinator_topics_;
        AgentTopics agent_topics_;
        OperatorTopics operator_topics_;

    private: // Frame data
        std::string world_frame_;
        AgentFrames agent_frames_;

    private: // ROS components
        // Node
        NodePtr node_;
        const std::string node_name_;
        // Callback group
        CallbackGroupPtr node_cb_group_;
        rclcpp::SubscriptionOptions sub_options_with_node_cb_group_;
        // TF2 components
        BufferPtr tf_buffer_;
        ListenerPtr tf_listener_;
        BroadcasterPtr tf_broadcaster_;
        StaticBroadcasterPtr static_tf_broadcaster_;

    public: // Topic getters
        std::string getRegistrationTopic();
        std::string getFleetStatusTopic();
        std::string getMissionStatusTopic();
        std::string getGlobalOriginTopic();
        std::string getTargetPositionTopic(const ID& target_id);
        std::string getClusterAssignmentTopic(const ID& cluster_id);
        std::string getClusterGeometryTopic(const ID& cluster_id);
        std::string getAgentStatusTopic(const ID& agent_id);
        std::string getAgentGlobalPositionTopic(const ID& agent_id);
        std::string getAgentLocalPositionTopic(const ID& agent_id);
        std::string getAgentAssignmentTopic(const ID& agent_id);
        std::string getAgentClustersTopic(const ID& agent_id);
        std::string getAgentPositionSetpointTopic(const ID& agent_id);
        std::string getObservationSetpointsTopic(const ID& agent_id);
        std::string getAgentMultiCameraImageTopic(const ID& agent_id, const ID& camera_id);
        std::string getAgentMultiWindowImageTopic(const ID& agent_id, const ID& window_id);
        std::string getGlobalMetricsTopic();
        std::string getAgentMetricsTopic(const ID& agent_id);
        std::string getTargetMetricsTopic(const ID& target_id);
        std::string getClusterMetricsTopic(const ID& cluster_id);
        std::string getAgentMarkersTopic(const ID& agent_id);
        std::string getTargetMarkersTopic(const ID& target_id);
        std::string getClusterMarkersTopic(const ID& cluster_id);

    public: // Publisher creation
        PublisherPtr<RegistrationMsg> createRegistrationPublisher();
        PublisherPtr<FleetStatusMsg> createFleetStatusPublisher();
        PublisherPtr<MissionStatusMsg> createMissionStatusPublisher();
        PublisherPtr<GeoPointStampedMsg> createGlobalOriginPublisher();
        PublisherPtr<PointStampedMsg> createTargetPositionPublisher(const ID& target_id);
        PublisherPtr<ClusterAssignmentMsg> createClusterAssignmentPublisher(const ID& cluster_id);
        PublisherPtr<ClusterGeometryMsg> createClusterGeometryPublisher(const ID& cluster_id);
        PublisherPtr<AgentStatusMsg> createAgentStatusPublisher(const ID& agent_id);
        PublisherPtr<PointStampedMsg> createAgentGlobalPositionPublisher(const ID& agent_id);
        PublisherPtr<PointStampedMsg> createAgentLocalPositionPublisher(const ID& agent_id);
        PublisherPtr<AgentAssignmentMsg> createAgentAssignmentPublisher(const ID& agent_id);
        PublisherPtr<AgentClustersMsg> createAgentClustersPublisher(const ID& agent_id);
        PublisherPtr<PointStampedMsg> createAgentPositionSetpointPublisher(const ID& agent_id);
        PublisherPtr<ObservationSetpointsMsg> createObservationSetpointsPublisher(const ID& agent_id);
        PublisherPtr<CompressedImageMsg> createAgentMultiCameraImagePublisher(const ID& agent_id, const ID& camera_id);
        PublisherPtr<CompressedImageMsg> createAgentMultiWindowImagePublisher(const ID& agent_id, const ID& window_id);
        PublisherPtr<MissionMetricsMsg> createMissionMetricsPublisher();
        PublisherPtr<AgentMetricsMsg> createAgentMetricsPublisher(const ID& agent_id);
        PublisherPtr<TargetMetricsMsg> createTargetMetricsPublisher(const ID& target_id);
        PublisherPtr<ClusterMetricsMsg> createClusterMetricsPublisher(const ID& cluster_id);
        PublisherPtr<MarkerArrayMsg> createAgentMarkersPublisher(const ID& agent_id);
        PublisherPtr<MarkerArrayMsg> createTargetMarkersPublisher(const ID& target_id);
        PublisherPtr<MarkerArrayMsg> createClusterMarkersPublisher(const ID& cluster_id);

    public: // Subscriber creation
        SubscriberPtr<RegistrationMsg> createRegistrationSubscriber(std::function<void(const RegistrationMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<FleetStatusMsg> createFleetStatusSubscriber(std::function<void(const FleetStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<MissionStatusMsg> createMissionStatusSubscriber(std::function<void(const MissionStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<GeoPointStampedMsg> createGlobalOriginSubscriber(std::function<void(const GeoPointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<PointStampedMsg> createTargetPositionSubscriber(const ID& target_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<ClusterAssignmentMsg> createClusterAssignmentSubscriber(const ID& cluster_id, std::function<void(const ClusterAssignmentMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<ClusterGeometryMsg> createClusterGeometrySubscriber(const ID& cluster_id, std::function<void(const ClusterGeometryMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<AgentStatusMsg> createAgentStatusSubscriber(const ID& agent_id, std::function<void(const AgentStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<PointStampedMsg> createAgentGlobalPositionSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<PointStampedMsg> createAgentLocalPositionSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<AgentAssignmentMsg> createAgentAssignmentSubscriber(const ID& agent_id, std::function<void(const AgentAssignmentMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<AgentClustersMsg> createAgentClustersSubscriber(const ID& agent_id, std::function<void(const AgentClustersMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<PointStampedMsg> createAgentPositionSetpointSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<ObservationSetpointsMsg> createObservationSetpointsSubscriber(const ID& agent_id, std::function<void(const ObservationSetpointsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<CompressedImageMsg> createAgentMultiCameraImageSubscriber(const ID& agent_id, const ID& camera_id, std::function<void(const CompressedImageMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<CompressedImageMsg> createAgentMultiWindowImageSubscriber(const ID& agent_id, const ID& window_id, std::function<void(const CompressedImageMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<MissionMetricsMsg> createGlobalMetricsSubscriber(std::function<void(const MissionMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<AgentMetricsMsg> createAgentMetricsSubscriber(const ID& agent_id, std::function<void(const AgentMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<TargetMetricsMsg> createTargetMetricsSubscriber(const ID& target_id, std::function<void(const TargetMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<ClusterMetricsMsg> createClusterMetricsSubscriber(const ID& cluster_id, std::function<void(const ClusterMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<MarkerArrayMsg> createAgentMarkersSubscriber(const ID& agent_id, std::function<void(const MarkerArrayMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<MarkerArrayMsg> createTargetMarkersSubscriber(const ID& target_id, std::function<void(const MarkerArrayMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<MarkerArrayMsg> createClusterMarkersSubscriber(const ID& cluster_id, std::function<void(const MarkerArrayMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());

    public: // Transform frame getters
        std::string getGlobalFrame();
        std::string getAgentLocalFrame(const ID& agent_id);
        std::string getAgentBodyFrame(const ID& agent_id);
        std::string getCameraBodyFrame(const ID& agent_id, const ID& camera_id);
        std::string getCameraOpticalFrame(const ID& agent_id, const ID& camera_id);

    public: // Transform utilities
        TransformMsg getTransform(const std::string& from_frame, const std::string& to_frame);
        PoseStampedMsg transformPose(const PoseStampedMsg& pose, const std::string& to_frame);
        PointStampedMsg transformPoint(const PointStampedMsg& point, const std::string& to_frame);

    public: // Transform broadcast utilities
        void broadcastTransform(const std::string& from_frame, const std::string& to_frame, const Matrix4r& transform);
        void broadcastStaticTransform(const std::string& from_frame, const std::string& to_frame, const Matrix4r& transform);
    };

} // namespace flychams::core