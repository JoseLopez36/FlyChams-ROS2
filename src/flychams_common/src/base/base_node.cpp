#include "flychams_common/base/base_node.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

BaseNode::BaseNode(const std::string& node_name, const rclcpp::NodeOptions& options)
    : Node(node_name, options), node_name_(node_name)
{
    // Nothing to do
}

void BaseNode::init()
{
    // Get node pointer
    node_ = this->shared_from_this();
    RCLCPP_INFO(node_->get_logger(), "Starting %s node...", node_name_.c_str());

    // Create callback group
    node_cb_group_ = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    sub_options_with_node_cb_group_.callback_group = node_cb_group_;

    // Create settings tools
    settings_ = std::make_shared<SettingsTools>(node_);

    // Initialize topic data
    const auto& topic_config = settings_->getTopics();

    // Coordinator topics
    coordinator_topics_.registration = topic_config.registration;
    coordinator_topics_.fleet_status = topic_config.fleet_status;
    coordinator_topics_.mission_status = topic_config.mission_status;
    coordinator_topics_.global_origin = topic_config.global_origin;
    coordinator_topics_.target_position_pattern = topic_config.target_position;
    coordinator_topics_.cluster_assignment_pattern = topic_config.cluster_assignment;
    coordinator_topics_.cluster_geometry_pattern = topic_config.cluster_geometry;
    coordinator_topics_.agent_assignment_pattern = topic_config.agent_assignment;
    coordinator_topics_.agent_clusters_pattern = topic_config.agent_clusters;
    coordinator_topics_.assignment_solve_duration = topic_config.assignment_solve_duration;

    // Agent topics
    agent_topics_.status_pattern = topic_config.agent_status;
    agent_topics_.global_position_pattern = topic_config.agent_global_position;
    agent_topics_.local_position_pattern = topic_config.agent_local_position;
    agent_topics_.position_setpoint_pattern = topic_config.agent_position_setpoint;
    agent_topics_.position_solve_duration_pattern = topic_config.position_solve_duration;
    agent_topics_.observation_setpoints_pattern = topic_config.observation_setpoints;
    agent_topics_.image_pattern = topic_config.image;
    agent_topics_.image_compressed_pattern = topic_config.image_compressed;
    agent_topics_.camera_info_pattern = topic_config.camera_info;

    // Simulation topics
    simulation_topics_.simulation_image_pattern = topic_config.simulation_image;

    // Operator topics
    operator_topics_.annotations_pattern = topic_config.annotations;
    operator_topics_.scene_pattern = topic_config.scene;
    operator_topics_.start_mission = topic_config.start_mission;
    operator_topics_.pause_mission = topic_config.pause_mission;
    operator_topics_.abort_mission = topic_config.abort_mission;
    operator_topics_.arm_all = topic_config.arm_all;
    operator_topics_.land_all = topic_config.land_all;
    operator_topics_.return_home = topic_config.return_home;
    operator_topics_.mission_metrics = topic_config.mission_metrics;
    operator_topics_.fleet_metrics = topic_config.fleet_metrics;
    operator_topics_.agent_metrics_pattern = topic_config.agent_metrics;
    operator_topics_.target_metrics_pattern = topic_config.target_metrics;
    operator_topics_.cluster_metrics_pattern = topic_config.cluster_metrics;

    // Get frame config
    const auto& frame_config = settings_->getFrames();
    world_frame_ = frame_config.world;
    agent_frames_.agent_local_pattern_ = frame_config.agent_local;
    agent_frames_.agent_body_pattern_ = frame_config.agent_body;
    agent_frames_.camera_body_pattern_ = frame_config.camera_body;
    agent_frames_.camera_optical_pattern_ = frame_config.camera_optical;

    // Initialize image transport
    image_transport_ = std::make_shared<image_transport::ImageTransport>(node_);

    // Call on init overridable method
    onNodeInit();
    RCLCPP_INFO(node_->get_logger(), "%s node running", node_name_.c_str());
}

BaseNode::~BaseNode()
{
    shutdown();
}

void BaseNode::shutdown()
{
    RCLCPP_INFO(node_->get_logger(), "Shutting down %s node...", node_name_.c_str());
    // Call on shutdown overridable method
    onNodeShutdown();
    // Destroy TF2 components
    tf_listener_.reset();
    tf_buffer_.reset();
    tf_broadcaster_.reset();
    static_tf_broadcaster_.reset();
    // Destroy settings tools
    settings_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// TF INITIALIZATION
// ════════════════════════════════════════════════════════════════════════════

void BaseNode::initTf()
{
    // Initialize TF2 components
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(node_->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
    tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(node_);
    static_tf_broadcaster_ = std::make_shared<tf2_ros::StaticTransformBroadcaster>(node_);
}

// ════════════════════════════════════════════════════════════════════════════
// TOPIC GETTERS
// ════════════════════════════════════════════════════════════════════════════

std::string BaseNode::getRegistrationTopic()
{
    return coordinator_topics_.registration;
}

std::string BaseNode::getFleetStatusTopic()
{
    return coordinator_topics_.fleet_status;
}

std::string BaseNode::getMissionStatusTopic()
{
    return coordinator_topics_.mission_status;
}

std::string BaseNode::getGlobalOriginTopic()
{
    return coordinator_topics_.global_origin;
}

std::string BaseNode::getTargetPositionTopic(const ID& target_id)
{
    return replace(coordinator_topics_.target_position_pattern, "TARGETID", target_id);
}

std::string BaseNode::getClusterAssignmentTopic(const ID& cluster_id)
{
    return replace(coordinator_topics_.cluster_assignment_pattern, "CLUSTERID", cluster_id);
}

std::string BaseNode::getClusterGeometryTopic(const ID& cluster_id)
{
    return replace(coordinator_topics_.cluster_geometry_pattern, "CLUSTERID", cluster_id);
}

std::string BaseNode::getAgentAssignmentTopic(const ID& agent_id)
{
    return replace(coordinator_topics_.agent_assignment_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getAgentClustersTopic(const ID& agent_id)
{
    return replace(coordinator_topics_.agent_clusters_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getAssignmentSolveDurationTopic()
{
    return coordinator_topics_.assignment_solve_duration;
}

std::string BaseNode::getAgentStatusTopic(const ID& agent_id)
{
    return replace(agent_topics_.status_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getAgentGlobalPositionTopic(const ID& agent_id)
{
    return replace(agent_topics_.global_position_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getAgentLocalPositionTopic(const ID& agent_id)
{
    return replace(agent_topics_.local_position_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getAgentPositionSetpointTopic(const ID& agent_id)
{
    return replace(agent_topics_.position_setpoint_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getObservationSetpointsTopic(const ID& agent_id)
{
    return replace(agent_topics_.observation_setpoints_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getPositionSolveDurationTopic(const ID& agent_id)
{
    return replace(agent_topics_.position_solve_duration_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getImageTopic(const ID& agent_id, const ID& unit_id)
{
    return replace(replace(agent_topics_.image_pattern, "AGENTID", agent_id), "UNITID", unit_id);
}

std::string BaseNode::getImageCompressedTopic(const ID& agent_id, const ID& unit_id)
{
    return replace(replace(agent_topics_.image_compressed_pattern, "AGENTID", agent_id), "UNITID", unit_id);
}

std::string BaseNode::getCameraInfoTopic(const ID& agent_id, const ID& unit_id)
{
    return replace(replace(agent_topics_.camera_info_pattern, "AGENTID", agent_id), "UNITID", unit_id);
}

std::string BaseNode::getSimulationImageTopic(const ID& view_id)
{
    return replace(simulation_topics_.simulation_image_pattern, "VIEWID", view_id);
}

std::string BaseNode::getAnnotationsTopic(const ID& agent_id, const ID& unit_id)
{
    return replace(replace(operator_topics_.annotations_pattern, "AGENTID", agent_id), "UNITID", unit_id);
}

std::string BaseNode::getSceneTopic()
{
    return operator_topics_.scene_pattern;
}

std::string BaseNode::getMissionMetricsTopic()
{
    return operator_topics_.mission_metrics;
}

std::string BaseNode::getFleetMetricsTopic()
{
    return operator_topics_.fleet_metrics;
}

std::string BaseNode::getStartMissionTopic() 
{ 
    return operator_topics_.start_mission; 
}

std::string BaseNode::getPauseMissionTopic() 
{ 
    return operator_topics_.pause_mission; 
}

std::string BaseNode::getAbortMissionTopic() 
{ 
    return operator_topics_.abort_mission; 
}

std::string BaseNode::getArmAllTopic() 
{ 
    return operator_topics_.arm_all; 
}

std::string BaseNode::getLandAllTopic() 
{ 
    return operator_topics_.land_all; 
}

std::string BaseNode::getReturnHomeTopic() 
{ 
    return operator_topics_.return_home; 
}

std::string BaseNode::getAgentMetricsTopic(const ID& agent_id)
{
    return replace(operator_topics_.agent_metrics_pattern, "AGENTID", agent_id);
}

std::string BaseNode::getTargetMetricsTopic(const ID& target_id)
{
    return replace(operator_topics_.target_metrics_pattern, "TARGETID", target_id);
}

std::string BaseNode::getClusterMetricsTopic(const ID& cluster_id)
{
    return replace(operator_topics_.cluster_metrics_pattern, "CLUSTERID", cluster_id);
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLISHER CREATION
// ════════════════════════════════════════════════════════════════════════════

PublisherPtr<RegistrationMsg> BaseNode::createRegistrationPublisher()
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_publisher<RegistrationMsg>(getRegistrationTopic(), qos);
}

PublisherPtr<FleetStatusMsg> BaseNode::createFleetStatusPublisher()
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_publisher<FleetStatusMsg>(getFleetStatusTopic(), qos);
}

PublisherPtr<MissionStatusMsg> BaseNode::createMissionStatusPublisher()
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_publisher<MissionStatusMsg>(getMissionStatusTopic(), qos);
}

PublisherPtr<GeoPointStampedMsg> BaseNode::createGlobalOriginPublisher()
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_publisher<GeoPointStampedMsg>(getGlobalOriginTopic(), qos);
}

PublisherPtr<PointStampedMsg> BaseNode::createTargetPositionPublisher(const ID& target_id)
{
    return node_->create_publisher<PointStampedMsg>(getTargetPositionTopic(target_id), 10);
}

PublisherPtr<ClusterAssignmentMsg> BaseNode::createClusterAssignmentPublisher(const ID& cluster_id)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_publisher<ClusterAssignmentMsg>(getClusterAssignmentTopic(cluster_id), qos);
}

PublisherPtr<ClusterGeometryMsg> BaseNode::createClusterGeometryPublisher(const ID& cluster_id)
{
    return node_->create_publisher<ClusterGeometryMsg>(getClusterGeometryTopic(cluster_id), 10);
}

PublisherPtr<AgentAssignmentMsg> BaseNode::createAgentAssignmentPublisher(const ID& agent_id)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_publisher<AgentAssignmentMsg>(getAgentAssignmentTopic(agent_id), qos);
}

PublisherPtr<AgentClustersMsg> BaseNode::createAgentClustersPublisher(const ID& agent_id)
{
    return node_->create_publisher<AgentClustersMsg>(getAgentClustersTopic(agent_id), 10);
}

PublisherPtr<Float32Msg> BaseNode::createAssignmentSolveDurationPublisher()
{
    return node_->create_publisher<Float32Msg>(getAssignmentSolveDurationTopic(), 10);
}

PublisherPtr<AgentStatusMsg> BaseNode::createAgentStatusPublisher(const ID& agent_id)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_publisher<AgentStatusMsg>(getAgentStatusTopic(agent_id), qos);
}

PublisherPtr<PointStampedMsg> BaseNode::createAgentGlobalPositionPublisher(const ID& agent_id)
{
    return node_->create_publisher<PointStampedMsg>(getAgentGlobalPositionTopic(agent_id), 10);
}

PublisherPtr<PointStampedMsg> BaseNode::createAgentLocalPositionPublisher(const ID& agent_id)
{
    return node_->create_publisher<PointStampedMsg>(getAgentLocalPositionTopic(agent_id), 10);
}

PublisherPtr<PointStampedMsg> BaseNode::createAgentPositionSetpointPublisher(const ID& agent_id)
{
    return node_->create_publisher<PointStampedMsg>(getAgentPositionSetpointTopic(agent_id), 10);
}

PublisherPtr<Float32Msg> BaseNode::createPositionSolveDurationPublisher(const ID& agent_id)
{
    return node_->create_publisher<Float32Msg>(getPositionSolveDurationTopic(agent_id), 10);
}

PublisherPtr<ObservationSetpointsMsg> BaseNode::createObservationSetpointsPublisher(const ID& agent_id)
{
    return node_->create_publisher<ObservationSetpointsMsg>(getObservationSetpointsTopic(agent_id), 10);
}

CameraPublisher BaseNode::createCameraPublisher(const ID& agent_id, const ID& unit_id)
{
    return image_transport_->advertiseCamera(getImageTopic(agent_id, unit_id), 1);
}

ImagePublisher BaseNode::createSimulationImagePublisher(const ID& view_id)
{
    return image_transport_->advertise(getSimulationImageTopic(view_id), 1);
}

PublisherPtr<FoxImageAnnotationsMsg> BaseNode::createAnnotationsPublisher(const ID& agent_id, const ID& unit_id)
{
    return node_->create_publisher<FoxImageAnnotationsMsg>(getAnnotationsTopic(agent_id, unit_id), 10);
}

PublisherPtr<FoxSceneUpdateMsg> BaseNode::createScenePublisher()
{
    return node_->create_publisher<FoxSceneUpdateMsg>(getSceneTopic(), 10);
}

PublisherPtr<BoolMsg> BaseNode::createStartMissionPublisher()
{
    return node_->create_publisher<BoolMsg>(getStartMissionTopic(), 10);
}

PublisherPtr<BoolMsg> BaseNode::createPauseMissionPublisher()
{
    return node_->create_publisher<BoolMsg>(getPauseMissionTopic(), 10);
}

PublisherPtr<BoolMsg> BaseNode::createAbortMissionPublisher()
{
    return node_->create_publisher<BoolMsg>(getAbortMissionTopic(), 10);
}

PublisherPtr<BoolMsg> BaseNode::createArmAllPublisher()
{
    return node_->create_publisher<BoolMsg>(getArmAllTopic(), 10);
}

PublisherPtr<BoolMsg> BaseNode::createLandAllPublisher()
{
    return node_->create_publisher<BoolMsg>(getLandAllTopic(), 10);
}

PublisherPtr<BoolMsg> BaseNode::createReturnHomePublisher()
{
    return node_->create_publisher<BoolMsg>(getReturnHomeTopic(), 10);
}

PublisherPtr<MissionMetricsMsg> BaseNode::createMissionMetricsPublisher()
{
    return node_->create_publisher<MissionMetricsMsg>(getMissionMetricsTopic(), 10);
}

PublisherPtr<FleetMetricsMsg> BaseNode::createFleetMetricsPublisher()
{
    return node_->create_publisher<FleetMetricsMsg>(getFleetMetricsTopic(), 10);
}

PublisherPtr<AgentMetricsMsg> BaseNode::createAgentMetricsPublisher(const ID& agent_id)
{
    return node_->create_publisher<AgentMetricsMsg>(getAgentMetricsTopic(agent_id), 10);
}

PublisherPtr<TargetMetricsMsg> BaseNode::createTargetMetricsPublisher(const ID& target_id)
{
    return node_->create_publisher<TargetMetricsMsg>(getTargetMetricsTopic(target_id), 10);
}

PublisherPtr<ClusterMetricsMsg> BaseNode::createClusterMetricsPublisher(const ID& cluster_id)
{
    return node_->create_publisher<ClusterMetricsMsg>(getClusterMetricsTopic(cluster_id), 10);
}

// ════════════════════════════════════════════════════════════════════════════
// SUBSCRIBER CREATION
// ════════════════════════════════════════════════════════════════════════════

SubscriberPtr<RegistrationMsg> BaseNode::createRegistrationSubscriber(std::function<void(const RegistrationMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_subscription<RegistrationMsg>(getRegistrationTopic(), qos, std::move(callback), options);
}

SubscriberPtr<FleetStatusMsg> BaseNode::createFleetStatusSubscriber(std::function<void(const FleetStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_subscription<FleetStatusMsg>(getFleetStatusTopic(), qos, std::move(callback), options);
}

SubscriberPtr<MissionStatusMsg> BaseNode::createMissionStatusSubscriber(std::function<void(const MissionStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_subscription<MissionStatusMsg>(getMissionStatusTopic(), qos, std::move(callback), options);
}

SubscriberPtr<GeoPointStampedMsg> BaseNode::createGlobalOriginSubscriber(std::function<void(const GeoPointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_subscription<GeoPointStampedMsg>(getGlobalOriginTopic(), qos, std::move(callback), options);
}

SubscriberPtr<PointStampedMsg> BaseNode::createTargetPositionSubscriber(const ID& target_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<PointStampedMsg>(getTargetPositionTopic(target_id), 10, std::move(callback), options);
}

SubscriberPtr<ClusterAssignmentMsg> BaseNode::createClusterAssignmentSubscriber(const ID& cluster_id, std::function<void(const ClusterAssignmentMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_subscription<ClusterAssignmentMsg>(getClusterAssignmentTopic(cluster_id), qos, std::move(callback), options);
}

SubscriberPtr<ClusterGeometryMsg> BaseNode::createClusterGeometrySubscriber(const ID& cluster_id, std::function<void(const ClusterGeometryMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<ClusterGeometryMsg>(getClusterGeometryTopic(cluster_id), 10, std::move(callback), options);
}

SubscriberPtr<AgentAssignmentMsg> BaseNode::createAgentAssignmentSubscriber(const ID& agent_id, std::function<void(const AgentAssignmentMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_subscription<AgentAssignmentMsg>(getAgentAssignmentTopic(agent_id), qos, std::move(callback), options);
}

SubscriberPtr<AgentClustersMsg> BaseNode::createAgentClustersSubscriber(const ID& agent_id, std::function<void(const AgentClustersMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<AgentClustersMsg>(getAgentClustersTopic(agent_id), 10, std::move(callback), options);
}

SubscriberPtr<Float32Msg> BaseNode::createAssignmentSolveDurationSubscriber(std::function<void(const Float32Msg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<Float32Msg>(getAssignmentSolveDurationTopic(), 10, std::move(callback), options);
}

SubscriberPtr<AgentStatusMsg> BaseNode::createAgentStatusSubscriber(const ID& agent_id, std::function<void(const AgentStatusMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    return node_->create_subscription<AgentStatusMsg>(getAgentStatusTopic(agent_id), qos, std::move(callback), options);
}

SubscriberPtr<PointStampedMsg> BaseNode::createAgentGlobalPositionSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<PointStampedMsg>(getAgentGlobalPositionTopic(agent_id), 10, std::move(callback), options);
}

SubscriberPtr<PointStampedMsg> BaseNode::createAgentLocalPositionSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<PointStampedMsg>(getAgentLocalPositionTopic(agent_id), 10, std::move(callback), options);
}

SubscriberPtr<PointStampedMsg> BaseNode::createAgentPositionSetpointSubscriber(const ID& agent_id, std::function<void(const PointStampedMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<PointStampedMsg>(getAgentPositionSetpointTopic(agent_id), 10, std::move(callback), options);
}

SubscriberPtr<Float32Msg> BaseNode::createPositionSolveDurationSubscriber(const ID& agent_id, std::function<void(const Float32Msg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<Float32Msg>(getPositionSolveDurationTopic(agent_id), 10, std::move(callback), options);
}

SubscriberPtr<ObservationSetpointsMsg> BaseNode::createObservationSetpointsSubscriber(const ID& agent_id, std::function<void(const ObservationSetpointsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<ObservationSetpointsMsg>(getObservationSetpointsTopic(agent_id), 10, std::move(callback), options);
}

SubscriberPtr<ImageMsg> BaseNode::createImageSubscriber(const ID& agent_id, const ID& unit_id, std::function<void(const ImageMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<ImageMsg>(getImageTopic(agent_id, unit_id), rclcpp::SensorDataQoS(), std::move(callback), options);
}

SubscriberPtr<CameraInfoMsg> BaseNode::createCameraInfoSubscriber(const ID& agent_id, const ID& unit_id, std::function<void(const CameraInfoMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<CameraInfoMsg>(getCameraInfoTopic(agent_id, unit_id), rclcpp::SensorDataQoS(), std::move(callback), options);
}

SubscriberPtr<BoolMsg> BaseNode::createStartMissionSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<BoolMsg>(getStartMissionTopic(), 10, std::move(callback), options);
}

SubscriberPtr<BoolMsg> BaseNode::createPauseMissionSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<BoolMsg>(getPauseMissionTopic(), 10, std::move(callback), options);
}

SubscriberPtr<BoolMsg> BaseNode::createAbortMissionSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<BoolMsg>(getAbortMissionTopic(), 10, std::move(callback), options);
}

SubscriberPtr<BoolMsg> BaseNode::createArmAllSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<BoolMsg>(getArmAllTopic(), 10, std::move(callback), options);
}

SubscriberPtr<BoolMsg> BaseNode::createLandAllSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<BoolMsg>(getLandAllTopic(), 10, std::move(callback), options);
}

SubscriberPtr<BoolMsg> BaseNode::createReturnHomeSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<BoolMsg>(getReturnHomeTopic(), 10, std::move(callback), options);
}

SubscriberPtr<MissionMetricsMsg> BaseNode::createMissionMetricsSubscriber(std::function<void(const MissionMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<MissionMetricsMsg>(getMissionMetricsTopic(), 10, std::move(callback), options);
}

SubscriberPtr<FleetMetricsMsg> BaseNode::createFleetMetricsSubscriber(std::function<void(const FleetMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<FleetMetricsMsg>(getFleetMetricsTopic(), 10, std::move(callback), options);
}

SubscriberPtr<AgentMetricsMsg> BaseNode::createAgentMetricsSubscriber(const ID& agent_id, std::function<void(const AgentMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<AgentMetricsMsg>(getAgentMetricsTopic(agent_id), 10, std::move(callback), options);
}

SubscriberPtr<TargetMetricsMsg> BaseNode::createTargetMetricsSubscriber(const ID& target_id, std::function<void(const TargetMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<TargetMetricsMsg>(getTargetMetricsTopic(target_id), 10, std::move(callback), options);
}

SubscriberPtr<ClusterMetricsMsg> BaseNode::createClusterMetricsSubscriber(const ID& cluster_id, std::function<void(const ClusterMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options)
{
    return node_->create_subscription<ClusterMetricsMsg>(getClusterMetricsTopic(cluster_id), 10, std::move(callback), options);
}

// ════════════════════════════════════════════════════════════════════════════
// TRANSFORM FRAME GETTERS
// ════════════════════════════════════════════════════════════════════════════

std::string BaseNode::getGlobalFrame()
{
    return world_frame_;
}

std::string BaseNode::getAgentLocalFrame(const ID& agent_id)
{
    return replace(agent_frames_.agent_local_pattern_, "AGENTID", agent_id);
}

std::string BaseNode::getAgentBodyFrame(const ID& agent_id)
{
    return replace(agent_frames_.agent_body_pattern_, "AGENTID", agent_id);
}

std::string BaseNode::getCameraBodyFrame(const ID& agent_id, const ID& camera_id)
{
    std::string pattern = replace(agent_frames_.camera_body_pattern_, "AGENTID", agent_id);
    return replace(pattern, "MULTICAMERAID", camera_id);
}

std::string BaseNode::getCameraOpticalFrame(const ID& agent_id, const ID& camera_id)
{
    std::string pattern = replace(agent_frames_.camera_optical_pattern_, "AGENTID", agent_id);
    return replace(pattern, "MULTICAMERAID", camera_id);
}

// ════════════════════════════════════════════════════════════════════════════
// TRANSFORM UTILITIES
// ════════════════════════════════════════════════════════════════════════════

TransformMsg BaseNode::getTransform(const std::string& from_frame, const std::string& to_frame)
{
    TransformMsg transform_msg;
    try {
        geometry_msgs::msg::TransformStamped transform_stamped =
            tf_buffer_->lookupTransform(from_frame, to_frame, tf2::TimePointZero);
        transform_msg.translation = transform_stamped.transform.translation;
        transform_msg.rotation = transform_stamped.transform.rotation;
    }
    catch (const tf2::TransformException& ex) {
        RCLCPP_WARN(node_->get_logger(), "Could not get the transform from %s to %s: %s",
            from_frame.c_str(), to_frame.c_str(), ex.what());
        transform_msg = TransformMsg();
        transform_msg.rotation.w = 1.0;
    }
    return transform_msg;
}

PoseStampedMsg BaseNode::transformPose(const PoseStampedMsg& pose, const std::string& to_frame)
{
    std::string from_frame = pose.header.frame_id;
    PoseStampedMsg transformed_pose = pose;
    transformed_pose.header.frame_id = to_frame;
    try {
        TransformStampedMsg transform = tf_buffer_->lookupTransform(
            to_frame, from_frame, tf2::TimePointZero);
        tf2::doTransform(pose, transformed_pose, transform);
    }
    catch (const tf2::TransformException& ex) {
        RCLCPP_WARN_THROTTLE(node_->get_logger(), *node_->get_clock(), 5000,
            "Failed to transform pose from %s to %s: %s",
            from_frame.c_str(), to_frame.c_str(), ex.what());
    }
    return transformed_pose;
}

PointStampedMsg BaseNode::transformPoint(const PointStampedMsg& point, const std::string& to_frame)
{
    std::string from_frame = point.header.frame_id;
    PointStampedMsg transformed_point = point;
    transformed_point.header.frame_id = to_frame;
    try {
        TransformStampedMsg transform = tf_buffer_->lookupTransform(
            to_frame, from_frame, tf2::TimePointZero);
        tf2::doTransform(point, transformed_point, transform);
    }
    catch (const tf2::TransformException& ex) {
        RCLCPP_WARN_THROTTLE(node_->get_logger(), *node_->get_clock(), 5000,
            "Failed to transform point from %s to %s: %s",
            from_frame.c_str(), to_frame.c_str(), ex.what());
    }
    return transformed_point;
}

// ════════════════════════════════════════════════════════════════════════════
// TRANSFORM BROADCAST UTILITIES
// ════════════════════════════════════════════════════════════════════════════

void BaseNode::broadcastTransform(const std::string& from_frame, const std::string& to_frame, const Matrix4r& transform)
{
    geometry_msgs::msg::TransformStamped transform_msg;
    transform_msg.header.stamp = now();
    transform_msg.header.frame_id = from_frame;
    transform_msg.child_frame_id = to_frame;
    toMsg(transform, transform_msg.transform);
    tf_broadcaster_->sendTransform(transform_msg);
}

void BaseNode::broadcastStaticTransform(const std::string& from_frame, const std::string& to_frame, const Matrix4r& transform)
{
    geometry_msgs::msg::TransformStamped transform_msg;
    transform_msg.header.stamp = now();
    transform_msg.header.frame_id = from_frame;
    transform_msg.child_frame_id = to_frame;
    toMsg(transform, transform_msg.transform);
    static_tf_broadcaster_->sendTransform(transform_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// TIMER UTILITIES
// ════════════════════════════════════════════════════════════════════════════

Time BaseNode::now()
{
    return node_->get_clock()->now();
}

TimerPtr BaseNode::createTimer(float rate_hz, std::function<void()> callback)
{
    return rclcpp::create_timer(
        node_,
        node_->get_clock(),
        std::chrono::duration<float>(1.0f / rate_hz),
        std::move(callback),
        node_cb_group_);
}

// ════════════════════════════════════════════════════════════════════════════
// MESSAGE UTILITIES
// ════════════════════════════════════════════════════════════════════════════

Vector3r BaseNode::fromMsg(const PointMsg& point)
{
    return Vector3r{ static_cast<float>(point.x), static_cast<float>(point.y), static_cast<float>(point.z) };
}

Vector3r BaseNode::fromMsg(const Vector3Msg& vector)
{
    return Vector3r{ static_cast<float>(vector.x), static_cast<float>(vector.y), static_cast<float>(vector.z) };
}

Quaternionr BaseNode::fromMsg(const QuaternionMsg& quat)
{
    return Quaternionr{ static_cast<float>(quat.w), static_cast<float>(quat.x), static_cast<float>(quat.y), static_cast<float>(quat.z) };
}

Matrix4r BaseNode::fromMsg(const TransformMsg& transform)
{
    Matrix4r T = Matrix4r::Identity();
    T.block<3, 1>(0, 3) = fromMsg(transform.translation);
    Quaternionr q = fromMsg(transform.rotation);
    T.block<3, 3>(0, 0) = MathUtils::quatToMatrix(q);
    return T;
}

void BaseNode::toMsg(const Vector3r& vector, PointMsg& point)
{
    point.x = static_cast<double>(vector.x());
    point.y = static_cast<double>(vector.y());
    point.z = static_cast<double>(vector.z());
}

void BaseNode::toMsg(const Vector3r& vector, Vector3Msg& vec)
{
    vec.x = static_cast<double>(vector.x());
    vec.y = static_cast<double>(vector.y());
    vec.z = static_cast<double>(vector.z());
}

void BaseNode::toMsg(const Quaternionr& orientation, QuaternionMsg& quat)
{
    quat.x = static_cast<double>(orientation.x());
    quat.y = static_cast<double>(orientation.y());
    quat.z = static_cast<double>(orientation.z());
    quat.w = static_cast<double>(orientation.w());
}

void BaseNode::toMsg(const Matrix4r& matrix, TransformMsg& transform)
{
    toMsg(matrix.block<3, 1>(0, 3), transform.translation);
    toMsg(MathUtils::quatFromMatrix(matrix.block<3, 3>(0, 0)), transform.rotation);
}

void BaseNode::toMsg(const Crop& crop, CropMsg& crop_msg)
{
    crop_msg.x = crop.x;
    crop_msg.y = crop.y;
    crop_msg.w = crop.w;
    crop_msg.h = crop.h;
    crop_msg.is_out_of_bounds = crop.is_out_of_bounds;
}

// ════════════════════════════════════════════════════════════════════════════
// OTHER UTILITIES
// ════════════════════════════════════════════════════════════════════════════

std::string BaseNode::replace(const std::string& topic_name, const std::string& placeholder, const std::string& value)
{
    return std::regex_replace(topic_name, std::regex(placeholder), value);
}

HeaderMsg BaseNode::createHeader(const std::string& frame_id)
{
    HeaderMsg header;
    header.frame_id = frame_id;
    header.stamp = now();
    return header;
}

bool BaseNode::addToSet(std::unordered_set<ID>& set, const ID& id)
{
    // Check if element already exists
    if (set.find(id) != set.end())
    {
        RCLCPP_INFO(node_->get_logger(), "Element %s already exists. Skipping addition", id.c_str());
        return false;
    }
    // Insert element
    set.insert(id);
    return true;
}

bool BaseNode::removeFromSet(std::unordered_set<ID>& set, const ID& id)
{
    // Check if element exists
    if (set.find(id) == set.end())
    {
        RCLCPP_INFO(node_->get_logger(), "Element %s does not exist. Skipping removal", id.c_str());
        return false;
    }
    // Remove element
    set.erase(id);
    return true;
}