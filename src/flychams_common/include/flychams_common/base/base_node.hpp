#pragma once

// Standard includes
#include <regex>

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
#include "flychams_common/utils/frame_utils.hpp"

namespace flychams::common
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
            std::string image_pattern;
            std::string image_compressed_pattern;
            std::string camera_info_pattern;
        };
        struct SimulationTopics
        {
            std::string simulation_image_pattern;
        };
        struct OperatorTopics
        {
            std::string annotations_pattern;
            std::string scene_pattern;
            std::string start_mission;
            std::string pause_mission;
            std::string abort_mission;
            std::string arm_all;
            std::string land_all;
            std::string return_home;
            std::string mission_metrics;
            std::string fleet_metrics;
            std::string agent_metrics_pattern;
            std::string target_metrics_pattern;
            std::string cluster_metrics_pattern;
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

    public: // Shared from this
        SharedPtr sharedFromThis()
        {
            return std::dynamic_pointer_cast<BaseNode>(shared_from_this());
        }

    public: // TF initialization
        void initTf();

    protected: // Settings data
        SettingsTools::SharedPtr settings_;

    private: // Topic data
        CoordinatorTopics coordinator_topics_;
        AgentTopics agent_topics_;
        SimulationTopics simulation_topics_;
        OperatorTopics operator_topics_;

    private: // Frame data
        std::string world_frame_;
        AgentFrames agent_frames_;

    protected: // ROS components
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
        // Image transport
        ImageTransportPtr image_transport_;

    public: // Settings getters
        SettingsTools::SharedPtr getSettings() { return settings_; }

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
        std::string getImageTopic(const ID& agent_id, const ID& unit_id);
        std::string getImageCompressedTopic(const ID& agent_id, const ID& unit_id);
        std::string getCameraInfoTopic(const ID& agent_id, const ID& unit_id);
        std::string getSimulationImageTopic(const ID& view_id);
        std::string getAnnotationsTopic(const ID& agent_id, const ID& unit_id);
        std::string getSceneTopic();
        std::string getStartMissionTopic();
        std::string getPauseMissionTopic();
        std::string getAbortMissionTopic();
        std::string getArmAllTopic();
        std::string getLandAllTopic();
        std::string getReturnHomeTopic();
        std::string getMissionMetricsTopic();
        std::string getFleetMetricsTopic();
        std::string getAgentMetricsTopic(const ID& agent_id);
        std::string getTargetMetricsTopic(const ID& target_id);
        std::string getClusterMetricsTopic(const ID& cluster_id);

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
        CameraPublisher createCameraPublisher(const ID& agent_id, const ID& unit_id);
        ImagePublisher createSimulationImagePublisher(const ID& view_id);
        PublisherPtr<FoxImageAnnotationsMsg> createAnnotationsPublisher(const ID& agent_id, const ID& unit_id);
        PublisherPtr<FoxSceneUpdateMsg> createScenePublisher();
        PublisherPtr<BoolMsg> createStartMissionPublisher();
        PublisherPtr<BoolMsg> createPauseMissionPublisher();
        PublisherPtr<BoolMsg> createAbortMissionPublisher();
        PublisherPtr<BoolMsg> createArmAllPublisher();
        PublisherPtr<BoolMsg> createLandAllPublisher();
        PublisherPtr<BoolMsg> createReturnHomePublisher();
        PublisherPtr<MissionMetricsMsg> createMissionMetricsPublisher();
        PublisherPtr<FleetMetricsMsg> createFleetMetricsPublisher();
        PublisherPtr<AgentMetricsMsg> createAgentMetricsPublisher(const ID& agent_id);
        PublisherPtr<TargetMetricsMsg> createTargetMetricsPublisher(const ID& target_id);
        PublisherPtr<ClusterMetricsMsg> createClusterMetricsPublisher(const ID& cluster_id);

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
        SubscriberPtr<ImageMsg> createImageSubscriber(const ID& agent_id, const ID& unit_id, std::function<void(const ImageMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<CameraInfoMsg> createCameraInfoSubscriber(const ID& agent_id, const ID& unit_id, std::function<void(const CameraInfoMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<BoolMsg> createStartMissionSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<BoolMsg> createPauseMissionSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<BoolMsg> createAbortMissionSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<BoolMsg> createArmAllSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<BoolMsg> createLandAllSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<BoolMsg> createReturnHomeSubscriber(std::function<void(const BoolMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<MissionMetricsMsg> createMissionMetricsSubscriber(std::function<void(const MissionMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<FleetMetricsMsg> createFleetMetricsSubscriber(std::function<void(const FleetMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<AgentMetricsMsg> createAgentMetricsSubscriber(const ID& agent_id, std::function<void(const AgentMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<TargetMetricsMsg> createTargetMetricsSubscriber(const ID& target_id, std::function<void(const TargetMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());
        SubscriberPtr<ClusterMetricsMsg> createClusterMetricsSubscriber(const ID& cluster_id, std::function<void(const ClusterMetricsMsg::SharedPtr)> callback, const rclcpp::SubscriptionOptions& options = rclcpp::SubscriptionOptions());

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

    public: // Callback group utilities
        CallbackGroupPtr getCallbackGroup() { return node_cb_group_; }
        rclcpp::SubscriptionOptions getSubscriptionOptions() { return sub_options_with_node_cb_group_; }

    public: // Timer utilities
        // Get the current time
        Time now();
        // Create a timer with the node's callback group
        TimerPtr createTimer(float rate_hz, std::function<void()> callback);

    public: // Parameter utilities
        // Get a parameter from the parameter server or shutdown the node
        template <typename T>
        T getParameter(const std::string& param_name)
        {
            T value;
            if (!node_->get_parameter(param_name, value))
            {
                RCLCPP_ERROR(node_->get_logger(), "Failed to get parameter '%s'. Shutting down node '%s'", param_name.c_str(), node_->get_name());
                throw rclcpp::exceptions::ParameterNotDeclaredException(param_name);
            }
            return value;
        }

        // Get a parameter from the parameter server or a default value
        template <typename T>
        T getParameterOr(const std::string& param_name, const T& default_value)
        {
            return node_->get_parameter_or(param_name, default_value);
        }

        // Set a parameter in the parameter server
        template <typename T>
        void setParameter(const std::string& param_name, const T& value)
        {
            node_->set_parameter(rclcpp::Parameter(param_name, value));
        }

    public: // Service utilities
        // Send a request to a service and wait for the response
        template<typename T>
        bool sendRequest(ClientPtr<T> client, typename T::Request::SharedPtr request, int wait_time_ms = 1000)
        {
            // First, wait for service to be available
            if (!client->wait_for_service(std::chrono::milliseconds(wait_time_ms)))
            {
                RCLCPP_ERROR(node_->get_logger(), "Service %s wait timed out", client->get_service_name());
                return false;
            }

            // Send the request and wait for the response
            client->async_send_request(request);
            return true;
        }

    public: // Message utilities
        // Convert a PointMsg to a Vector3r
        Vector3r fromMsg(const PointMsg& point);

        // Convert a Vector3Msg to a Vector3r
        Vector3r fromMsg(const Vector3Msg& vector);

        // Convert a QuaternionMsg to a Quaternionr
        Quaternionr fromMsg(const QuaternionMsg& quat);

        // Convert a TransformMsg to a Matrix4r
        Matrix4r fromMsg(const TransformMsg& transform);

        // Convert a Vector3r to a PointMsg
        void toMsg(const Vector3r& vector, PointMsg& point);

        // Convert a Vector3r to a Vector3Msg
        void toMsg(const Vector3r& vector, Vector3Msg& vec);

        // Convert a Quaternionr to a QuaternionMsg
        void toMsg(const Quaternionr& orientation, QuaternionMsg& quat);

        // Convert a Matrix4r to a TransformMsg
        void toMsg(const Matrix4r& matrix, TransformMsg& transform);

        // Convert a Crop to a CropMsg
        void toMsg(const Crop& crop, CropMsg& crop_msg);

    public: // Other utilities
        // Replace a placeholder in a topic name
        std::string replace(const std::string& topic_name, const std::string& placeholder, const std::string& value);

        // Create a header
        HeaderMsg createHeader(const std::string& frame_id);

        // Add an element to a set
        bool addToSet(std::unordered_set<ID>& set, const ID& id);

        // Remove an element from a set
        bool removeFromSet(std::unordered_set<ID>& set, const ID& id);
    };

} // namespace flychams::common