#pragma once

// Tools includes
#include "flychams_common/settings/settings_tools.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Topic Manager for handling topics
     *
     * @details
     * This class provides utilities for managing topics
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-28
     * ════════════════════════════════════════════════════════════════
     */
    class TopicTools
    {
    public: // Constructor/Destructor
        TopicTools(NodePtr node, const SettingsTools::SharedPtr& settings_tools)
            : node_(node), settings_tools_(settings_tools)
        {
            // Get topic config
            const auto& topic_config = settings_tools_->getTopics();

            // Get coordinator topics
            coordinator_topics_.registration = topic_config.registration;
            coordinator_topics_.fleet_status = topic_config.fleet_status;
            coordinator_topics_.mission_status = topic_config.mission_status;
            coordinator_topics_.global_origin = topic_config.global_origin;
            coordinator_topics_.target_position_pattern = topic_config.target_position;
            coordinator_topics_.cluster_assignment_pattern = topic_config.cluster_assignment;
            coordinator_topics_.cluster_geometry_pattern = topic_config.cluster_geometry;

            // Get agent topics
            agent_topics_.status_pattern = topic_config.agent_status;
            agent_topics_.global_position_pattern = topic_config.agent_global_position;
            agent_topics_.local_position_pattern = topic_config.agent_local_position;
            agent_topics_.assignment_pattern = topic_config.agent_assignment;
            agent_topics_.clusters_pattern = topic_config.agent_clusters;
            agent_topics_.position_setpoint_pattern = topic_config.agent_position_setpoint;
            agent_topics_.observation_setpoints_pattern = topic_config.observation_setpoints;
            agent_topics_.multi_camera_image_pattern = topic_config.agent_multi_camera_image;
            agent_topics_.multi_window_image_pattern = topic_config.agent_multi_window_image;

            // Get operator topics
            operator_topics_.metrics = topic_config.mission_metrics;
            operator_topics_.agent_metrics_pattern = topic_config.agent_metrics;
            operator_topics_.target_metrics_pattern = topic_config.target_metrics;
            operator_topics_.cluster_metrics_pattern = topic_config.cluster_metrics;
            operator_topics_.agent_markers_pattern = topic_config.agent_markers;
            operator_topics_.target_markers_pattern = topic_config.target_markers;
            operator_topics_.cluster_markers_pattern = topic_config.cluster_markers;
        }

        ~TopicTools()
        {
            shutdown();
        }

        void shutdown()
        {
            // Destroy config tools
            settings_tools_.reset();
            // Destroy node
            node_.reset();
        }

    public: // Types
        using SharedPtr = std::shared_ptr<TopicTools>;
        // Coordinator topics
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
        // Agent topics
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
        // Operator topics
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

    private: // Data
        // Topics
        CoordinatorTopics coordinator_topics_;
        AgentTopics agent_topics_;
        OperatorTopics operator_topics_;

        // ROS components
        NodePtr node_;

        // Config tools
        SettingsTools::SharedPtr settings_tools_;

    public: // Topic getters
        // Coordinator topics
        std::string getRegistrationTopic()
        {
            return coordinator_topics_.registration;
        }
        std::string getFleetStatusTopic()
        {
            return fleet_topics_.fleet_status;
        }
        std::string getMissionStatusTopic()
        {
            return fleet_topics_.mission_status;
        }
        std::string getGlobalOriginTopic()
        {
            return coordinator_topics_.global_origin;
        }
        std::string getTargetPositionTopic(const ID& target_id)
        {
            return RosUtils::replace(coordinator_topics_.target_position_pattern, "TARGETID", target_id);
        }
        std::string getClusterAssignmentTopic(const ID& cluster_id)
        {
            return RosUtils::replace(coordinator_topics_.cluster_assignment_pattern, "CLUSTERID", cluster_id);
        }
        std::string getClusterGeometryTopic(const ID& cluster_id)
        {
            return RosUtils::replace(coordinator_topics_.cluster_geometry_pattern, "CLUSTERID", cluster_id);
        }

        // Agent topics
        std::string getAgentStatusTopic(const ID& agent_id)
        {
            return RosUtils::replace(agent_topics_.status_pattern, "AGENTID", agent_id);
        }
        std::string getAgentGlobalPositionTopic(const ID& agent_id)
        {
            return RosUtils::replace(agent_topics_.global_position_pattern, "AGENTID", agent_id);
        }
        std::string getAgentLocalPositionTopic(const ID& agent_id)
        {
            return RosUtils::replace(agent_topics_.local_position_pattern, "AGENTID", agent_id);
        }
        std::string getAgentAssignmentTopic(const ID& agent_id)
        {
            return RosUtils::replace(agent_topics_.assignment_pattern, "AGENTID", agent_id);
        }
        std::string getAgentClustersTopic(const ID& agent_id)
        {
            return RosUtils::replace(agent_topics_.clusters_pattern, "AGENTID", agent_id);
        }
        std::string getAgentPositionSetpointTopic(const ID& agent_id)
        {
            return RosUtils::replace(agent_topics_.position_setpoint_pattern, "AGENTID", agent_id);
        }
        std::string getObservationSetpointsTopic(const ID& agent_id)
        {
            return RosUtils::replace(agent_topics_.observation_setpoints_pattern, "AGENTID", agent_id);
        }
        std::string getAgentMultiCameraImageTopic(const ID& agent_id, const ID& camera_id)
        {
            return RosUtils::replace(RosUtils::replace(agent_topics_.multi_camera_image_pattern, "AGENTID", agent_id), "MULTICAMERAID", camera_id);
        }
        std::string getAgentMultiWindowImageTopic(const ID& agent_id, const ID& window_id)
        {
            return RosUtils::replace(RosUtils::replace(agent_topics_.multi_window_image_pattern, "AGENTID", agent_id), "MULTIWINDOWID", window_id);
        }

        // Operator topics
        std::string getGlobalMetricsTopic()
        {
            return operator_topics_.metrics;
        }
        std::string getAgentMetricsTopic(const ID& agent_id)
        {
            return RosUtils::replace(operator_topics_.agent_metrics_pattern, "AGENTID", agent_id);
        }
        std::string getTargetMetricsTopic(const ID& target_id)
        {
            return RosUtils::replace(operator_topics_.target_metrics_pattern, "TARGETID", target_id);
        }
        std::string getClusterMetricsTopic(const ID& cluster_id)
        {
            return RosUtils::replace(operator_topics_.cluster_metrics_pattern, "CLUSTERID", cluster_id);
        }
        std::string getAgentMarkersTopic(const ID& agent_id)
        {
            return RosUtils::replace(operator_topics_.agent_markers_pattern, "AGENTID", agent_id);
        }
        std::string getTargetMarkersTopic(const ID& target_id)
        {
            return RosUtils::replace(operator_topics_.target_markers_pattern, "TARGETID", target_id);
        }
        std::string getClusterMarkersTopic(const ID& cluster_id)
        {
            return RosUtils::replace(operator_topics_.cluster_markers_pattern, "CLUSTERID", cluster_id);
        }

    public: // Topic creation utilities
        // Publishers
        // Coordinator publishers
        PublisherPtr<RegistrationMsg> createRegistrationPublisher()
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_publisher<RegistrationMsg>(getRegistrationTopic(), qos);
        }
        PublisherPtr<FleetStatusMsg> createFleetStatusPublisher()
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_publisher<FleetStatusMsg>(getFleetStatusTopic(), qos);
        }
        PublisherPtr<MissionStatusMsg> createMissionStatusPublisher()
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_publisher<MissionStatusMsg>(getMissionStatusTopic(), qos);
        }
        PublisherPtr<GeoPointStampedMsg> createGlobalOriginPublisher()
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_publisher<GeoPointStampedMsg>(getGlobalOriginTopic(), qos);
        }
        PublisherPtr<PointStampedMsg> createTargetPositionPublisher(const ID& target_id)
        {
            return node_->create_publisher<PointStampedMsg>(getTargetPositionTopic(target_id), 10);
        }
        PublisherPtr<ClusterAssignmentMsg> createClusterAssignmentPublisher(const ID& cluster_id)
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_publisher<ClusterAssignmentMsg>(getClusterAssignmentTopic(cluster_id), qos);
        }
        PublisherPtr<ClusterGeometryMsg> createClusterGeometryPublisher(const ID& cluster_id)
        {
            return node_->create_publisher<ClusterGeometryMsg>(getClusterGeometryTopic(cluster_id), 10);
        }

        // Agent publishers
        PublisherPtr<AgentStatusMsg> createAgentStatusPublisher(const ID& agent_id)
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_publisher<AgentStatusMsg>(getAgentStatusTopic(agent_id), qos);
        }
        PublisherPtr<PointStampedMsg> createAgentGlobalPositionPublisher(const ID& agent_id)
        {
            return node_->create_publisher<PointStampedMsg>(getAgentGlobalPositionTopic(agent_id), 10);
        }
        PublisherPtr<PointStampedMsg> createAgentLocalPositionPublisher(const ID& agent_id)
        {
            return node_->create_publisher<PointStampedMsg>(getAgentLocalPositionTopic(agent_id), 10);
        }
        PublisherPtr<AgentAssignmentMsg> createAgentAssignmentPublisher(const ID& agent_id)
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_publisher<AgentAssignmentMsg>(getAgentAssignmentTopic(agent_id), qos);
        }
        PublisherPtr<AgentClustersMsg> createAgentClustersPublisher(const ID& agent_id)
        {
            return node_->create_publisher<AgentClustersMsg>(getAgentClustersTopic(agent_id), 10);
        }
        PublisherPtr<PointStampedMsg> createAgentPositionSetpointPublisher(const ID& agent_id)
        {
            return node_->create_publisher<PointStampedMsg>(getAgentPositionSetpointTopic(agent_id), 10);
        }
        PublisherPtr<ObservationSetpointsMsg> createObservationSetpointsPublisher(const ID& agent_id)
        {
            return node_->create_publisher<ObservationSetpointsMsg>(getObservationSetpointsTopic(agent_id), 10);
        }
        PublisherPtr<CompressedImageMsg> createAgentMultiCameraImagePublisher(const ID& agent_id, const ID& camera_id)
        {
            return node_->create_publisher<CompressedImageMsg>(getAgentMultiCameraImageTopic(agent_id, camera_id), 10);
        }
        PublisherPtr<CompressedImageMsg> createAgentMultiWindowImagePublisher(const ID& agent_id, const ID& window_id)
        {
            return node_->create_publisher<CompressedImageMsg>(getAgentMultiWindowImageTopic(agent_id, window_id), 10);
        }

        // Operator publishers
        PublisherPtr<MissionMetricsMsg> createMissionMetricsPublisher()
        {
            return node_->create_publisher<MissionMetricsMsg>(getGlobalMetricsTopic(), 10);
        }
        PublisherPtr<AgentMetricsMsg> createAgentMetricsPublisher(const ID& agent_id)
        {
            return node_->create_publisher<AgentMetricsMsg>(getAgentMetricsTopic(agent_id), 10);
        }
        PublisherPtr<TargetMetricsMsg> createTargetMetricsPublisher(const ID& target_id)
        {
            return node_->create_publisher<TargetMetricsMsg>(getTargetMetricsTopic(target_id), 10);
        }
        PublisherPtr<ClusterMetricsMsg> createClusterMetricsPublisher(const ID& cluster_id)
        {
            return node_->create_publisher<ClusterMetricsMsg>(getClusterMetricsTopic(cluster_id), 10);
        }
        PublisherPtr<MarkerArrayMsg> createAgentMarkersPublisher(const ID& agent_id)
        {
            return node_->create_publisher<MarkerArrayMsg>(getAgentMarkersTopic(agent_id), 10);
        }
        PublisherPtr<MarkerArrayMsg> createTargetMarkersPublisher(const ID& target_id)
        {
            return node_->create_publisher<MarkerArrayMsg>(getTargetMarkersTopic(target_id), 10);
        }
        PublisherPtr<MarkerArrayMsg> createClusterMarkersPublisher(const ID& cluster_id)
        {
            return node_->create_publisher<MarkerArrayMsg>(getClusterMarkersTopic(cluster_id), 10);
        }

        // Subscribers
        // Coordinator subscribers
        SubscriberPtr<RegistrationMsg> createRegistrationSubscriber(std::function<void(const RegistrationMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_subscription<RegistrationMsg>(getRegistrationTopic(), qos, std::move(callback), options);
        }
        SubscriberPtr<FleetStatusMsg> createFleetStatusSubscriber(std::function<void(const FleetStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_subscription<FleetStatusMsg>(getFleetStatusTopic(), qos, std::move(callback), options);
        }
        SubscriberPtr<MissionStatusMsg> createMissionStatusSubscriber(std::function<void(const MissionStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_subscription<MissionStatusMsg>(getMissionStatusTopic(), qos, std::move(callback), options);
        }
        SubscriberPtr<GeoPointStampedMsg> createGlobalOriginSubscriber(std::function<void(const GeoPointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_subscription<GeoPointStampedMsg>(getGlobalOriginTopic(), qos, std::move(callback), options);
        }
        SubscriberPtr<PointStampedMsg> createTargetPositionSubscriber(const ID& target_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<PointStampedMsg>(getTargetPositionTopic(target_id), 10, std::move(callback), options);
        }
        SubscriberPtr<ClusterAssignmentMsg> createClusterAssignmentSubscriber(const ID& cluster_id, std::function<void(const ClusterAssignmentMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_subscription<ClusterAssignmentMsg>(getClusterAssignmentTopic(cluster_id), qos, std::move(callback), options);
        }
        SubscriberPtr<ClusterGeometryMsg> createClusterGeometrySubscriber(const ID& cluster_id, std::function<void(const ClusterGeometryMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<ClusterGeometryMsg>(getClusterGeometryTopic(cluster_id), 10, std::move(callback), options);
        }

        // Agent subscribers
        SubscriberPtr<AgentStatusMsg> createAgentStatusSubscriber(const ID& agent_id, std::function<void(const AgentStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_subscription<AgentStatusMsg>(getAgentStatusTopic(agent_id), qos, std::move(callback), options);
        }
        SubscriberPtr<PointStampedMsg> createAgentGlobalPositionSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<PointStampedMsg>(getAgentGlobalPositionTopic(agent_id), 10, std::move(callback), options);
        }
        SubscriberPtr<PointStampedMsg> createAgentLocalPositionSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<PointStampedMsg>(getAgentLocalPositionTopic(agent_id), 10, std::move(callback), options);
        }
        SubscriberPtr<AgentAssignmentMsg> createAgentAssignmentSubscriber(const ID& agent_id, std::function<void(const AgentAssignmentMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
            return node_->create_subscription<AgentAssignmentMsg>(getAgentAssignmentTopic(agent_id), qos, std::move(callback), options);
        }
        SubscriberPtr<AgentClustersMsg> createAgentClustersSubscriber(const ID& agent_id, std::function<void(const AgentClustersMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<AgentClustersMsg>(getAgentClustersTopic(agent_id), 10, std::move(callback), options);
        }
        SubscriberPtr<PointStampedMsg> createAgentPositionSetpointSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<PointStampedMsg>(getAgentPositionSetpointTopic(agent_id), 10, std::move(callback), options);
        }
        SubscriberPtr<ObservationSetpointsMsg> createObservationSetpointsSubscriber(const ID& agent_id, std::function<void(const ObservationSetpointsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<ObservationSetpointsMsg>(getObservationSetpointsTopic(agent_id), 10, std::move(callback), options);
        }
        SubscriberPtr<CompressedImageMsg> createAgentMultiCameraImageSubscriber(const ID& agent_id, const ID& camera_id, std::function<void(const CompressedImageMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<CompressedImageMsg>(getAgentMultiCameraImageTopic(agent_id, camera_id), 10, std::move(callback), options);
        }
        SubscriberPtr<CompressedImageMsg> createAgentMultiWindowImageSubscriber(const ID& agent_id, const ID& window_id, std::function<void(const CompressedImageMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<CompressedImageMsg>(getAgentMultiWindowImageTopic(agent_id, window_id), 10, std::move(callback), options);
        }

        // Operator subscribers
        SubscriberPtr<MissionMetricsMsg> createGlobalMetricsSubscriber(std::function<void(const MissionMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<MissionMetricsMsg>(getGlobalMetricsTopic(), 10, std::move(callback), options);
        }
        SubscriberPtr<AgentMetricsMsg> createAgentMetricsSubscriber(const ID& agent_id, std::function<void(const AgentMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<AgentMetricsMsg>(getAgentMetricsTopic(agent_id), 10, std::move(callback), options);
        }
        SubscriberPtr<TargetMetricsMsg> createTargetMetricsSubscriber(const ID& target_id, std::function<void(const TargetMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<TargetMetricsMsg>(getTargetMetricsTopic(target_id), 10, std::move(callback), options);
        }
        SubscriberPtr<ClusterMetricsMsg> createClusterMetricsSubscriber(const ID& cluster_id, std::function<void(const ClusterMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<ClusterMetricsMsg>(getClusterMetricsTopic(cluster_id), 10, std::move(callback), options);
        }
        SubscriberPtr<MarkerArrayMsg> createAgentMarkersSubscriber(const ID& agent_id, std::function<void(const MarkerArrayMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<MarkerArrayMsg>(getAgentMarkersTopic(agent_id), 10, std::move(callback), options);
        }
        SubscriberPtr<MarkerArrayMsg> createTargetMarkersSubscriber(const ID& target_id, std::function<void(const MarkerArrayMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<MarkerArrayMsg>(getTargetMarkersTopic(target_id), 10, std::move(callback), options);
        }
        SubscriberPtr<MarkerArrayMsg> createClusterMarkersSubscriber(const ID& cluster_id, std::function<void(const MarkerArrayMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions())
        {
            return node_->create_subscription<MarkerArrayMsg>(getClusterMarkersTopic(cluster_id), 10, std::move(callback), options);
        }
    };

} // namespace flychams::core 