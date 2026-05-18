#pragma once

// ROS includes
#include <rclcpp/rclcpp.hpp>

// Standard messages
#include <std_msgs/msg/header.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/empty.hpp>
#include <std_msgs/msg/bool.hpp>

// Geometry messages
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/quaternion.hpp>
#include <geometry_msgs/msg/transform.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include <geometry_msgs/msg/vector3_stamped.hpp>

// Navigation messages
#include <nav_msgs/msg/odometry.hpp>

// Sensor messages
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <sensor_msgs/msg/image.hpp>

// Image transport
#include <image_transport/image_transport.hpp>

// Geographic messages
#include <geographic_msgs/msg/geo_point.hpp>
#include <geographic_msgs/msg/geo_point_stamped.hpp>

// Visualization messages
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

// TF2 includes
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <tf2/transform_datatypes.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/LinearMath/Transform.h>
#include <tf2/LinearMath/Vector3.h>
#include <tf2/utils.h>

// Foxglove messages
#include <foxglove_msgs/msg/scene_update.hpp>
#include <foxglove_msgs/msg/scene_entity.hpp>
#include <foxglove_msgs/msg/scene_entity_deletion.hpp>
#include <foxglove_msgs/msg/sphere_primitive.hpp>
#include <foxglove_msgs/msg/cylinder_primitive.hpp>
#include <foxglove_msgs/msg/arrow_primitive.hpp>
#include <foxglove_msgs/msg/line_primitive.hpp>
#include <foxglove_msgs/msg/text_primitive.hpp>
#include <foxglove_msgs/msg/color.hpp>
#include <foxglove_msgs/msg/key_value_pair.hpp>
#include <foxglove_msgs/msg/image_annotations.hpp>
#include <foxglove_msgs/msg/circle_annotation.hpp>
#include <foxglove_msgs/msg/points_annotation.hpp>
#include <foxglove_msgs/msg/text_annotation.hpp>
#include <foxglove_msgs/msg/point2.hpp>

// Custom message types
// Base messages
#include "flychams_api/msg/element.hpp"
#include "flychams_api/msg/crop.hpp"
// Coordinator messages
#include "flychams_api/msg/registration.hpp"
#include "flychams_api/msg/fleet_status.hpp"
#include "flychams_api/msg/mission_status.hpp"
#include "flychams_api/msg/cluster_assignment.hpp"
#include "flychams_api/msg/cluster_geometry.hpp"
// Agent messages
#include "flychams_api/msg/agent_status.hpp"
#include "flychams_api/msg/agent_assignment.hpp"
#include "flychams_api/msg/agent_clusters.hpp"
#include "flychams_api/msg/observation_setpoints.hpp"
// Operator messages
#include "flychams_api/msg/mission_metrics.hpp"
#include "flychams_api/msg/agent_metrics.hpp"
#include "flychams_api/msg/target_metrics.hpp"
#include "flychams_api/msg/cluster_metrics.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief ROS types
     *
     * @details
     * This file contains all the ROS types used throughout the project,
     * including ROS message types, TF2 types, and custom message types.
     * ════════════════════════════════════════════════════════════════
     */

    // ════════════════════════════════════════════════════════════════
    // BASIC ROS TYPES: Basic ROS types
    // ════════════════════════════════════════════════════════════════

    // Node pointer
    using NodePtr = rclcpp::Node::SharedPtr;
    // Executor
    using ExecutorPtr = rclcpp::executors::SingleThreadedExecutor::SharedPtr;
    // TF2
    using BufferPtr = std::shared_ptr<tf2_ros::Buffer>;
    using ListenerPtr = std::shared_ptr<tf2_ros::TransformListener>;
    using BroadcasterPtr = std::shared_ptr<tf2_ros::TransformBroadcaster>;
    using StaticBroadcasterPtr = std::shared_ptr<tf2_ros::StaticTransformBroadcaster>;
    // Image transport
    using ImageTransportPtr = std::shared_ptr<image_transport::ImageTransport>;
    // Time
    using Time = rclcpp::Time;
    using TimerPtr = rclcpp::TimerBase::SharedPtr;
    // Publisher
    template<typename T>
    using PublisherPtr = typename rclcpp::Publisher<T>::SharedPtr;
    // Subscriber
    template<typename T>
    using SubscriberPtr = typename rclcpp::Subscription<T>::SharedPtr;
    // Callback group
    using CallbackGroupPtr = std::shared_ptr<rclcpp::CallbackGroup>;
    // Service
    template<typename T>
    using ServicePtr = typename rclcpp::Service<T>::SharedPtr;
    // Client
    template<typename T>
    using ClientPtr = typename rclcpp::Client<T>::SharedPtr;

    // ════════════════════════════════════════════════════════════════
    // MESSAGE TYPES: ROS message types
    // ════════════════════════════════════════════════════════════════

    // Header
    using HeaderMsg = std_msgs::msg::Header;
    // Float32
    using Float32Msg = std_msgs::msg::Float32;
    // Pose and twist
    using PointMsg = geometry_msgs::msg::Point;
    using PointStampedMsg = geometry_msgs::msg::PointStamped;
    using QuaternionMsg = geometry_msgs::msg::Quaternion;
    using PoseMsg = geometry_msgs::msg::Pose;
    using PoseStampedMsg = geometry_msgs::msg::PoseStamped;
    using TwistMsg = geometry_msgs::msg::Twist;
    using TwistStampedMsg = geometry_msgs::msg::TwistStamped;
    // Transform
    using Vector3Msg = geometry_msgs::msg::Vector3;
    using Vector3StampedMsg = geometry_msgs::msg::Vector3Stamped;
    using TransformMsg = geometry_msgs::msg::Transform;
    using TransformStampedMsg = geometry_msgs::msg::TransformStamped;
    // Odometry
    using OdometryMsg = nav_msgs::msg::Odometry;
    // NavSatFix
    using NavSatFixMsg = sensor_msgs::msg::NavSatFix;
    // Image
    using ImageMsg = sensor_msgs::msg::Image;
    // Image transport
    using ImagePublisher = image_transport::Publisher;
    // Geographic
    using GeoPointMsg = geographic_msgs::msg::GeoPoint;
    using GeoPointStampedMsg = geographic_msgs::msg::GeoPointStamped;
    // Marker
    using MarkerMsg = visualization_msgs::msg::Marker;
    using MarkerArrayMsg = visualization_msgs::msg::MarkerArray;
    // Color
    using ColorMsg = std_msgs::msg::ColorRGBA;
    // String
    using StringMsg = std_msgs::msg::String;
    // Empty
    using EmptyMsg = std_msgs::msg::Empty;
    // Bool
    using BoolMsg = std_msgs::msg::Bool;

    // ════════════════════════════════════════════════════════════════
    // TF2 TYPES: Transform types
    // ════════════════════════════════════════════════════════════════

    // Transform
    using TransformTf = tf2::Transform;
    // Vector3
    using Vector3Tf = tf2::Vector3;

    // ════════════════════════════════════════════════════════════════
    // FOXGLOVE MESSAGE TYPES: foxglove_msgs types
    // ════════════════════════════════════════════════════════════════

    // 3D scene
    using FoxSceneUpdateMsg = foxglove_msgs::msg::SceneUpdate;
    using FoxSceneEntityMsg = foxglove_msgs::msg::SceneEntity;
    using FoxSceneEntityDeletionMsg = foxglove_msgs::msg::SceneEntityDeletion;
    using FoxSpherePrimitiveMsg = foxglove_msgs::msg::SpherePrimitive;
    using FoxCylinderPrimitiveMsg = foxglove_msgs::msg::CylinderPrimitive;
    using FoxArrowPrimitiveMsg = foxglove_msgs::msg::ArrowPrimitive;
    using FoxLinePrimitiveMsg = foxglove_msgs::msg::LinePrimitive;
    using FoxTextPrimitiveMsg = foxglove_msgs::msg::TextPrimitive;
    using FoxColorMsg = foxglove_msgs::msg::Color;
    using FoxKeyValuePairMsg = foxglove_msgs::msg::KeyValuePair;
    // Image annotations
    using FoxImageAnnotationsMsg = foxglove_msgs::msg::ImageAnnotations;
    using FoxCircleAnnotationMsg = foxglove_msgs::msg::CircleAnnotation;
    using FoxPointsAnnotationMsg = foxglove_msgs::msg::PointsAnnotation;
    using FoxTextAnnotationMsg = foxglove_msgs::msg::TextAnnotation;
    using FoxPoint2Msg = foxglove_msgs::msg::Point2;

    // ════════════════════════════════════════════════════════════════
    // CUSTOM MESSAGE TYPES: FlyChams-specific message types
    // ════════════════════════════════════════════════════════════════

    // Base messages
    using ElementMsg = flychams_api::msg::Element;
    using CropMsg = flychams_api::msg::Crop;
    // Coordinator messages
    using RegistrationMsg = flychams_api::msg::Registration;
    using FleetStatusMsg = flychams_api::msg::FleetStatus;
    using MissionStatusMsg = flychams_api::msg::MissionStatus;
    using ClusterAssignmentMsg = flychams_api::msg::ClusterAssignment;
    using ClusterGeometryMsg = flychams_api::msg::ClusterGeometry;
    // Agent messages
    using AgentStatusMsg = flychams_api::msg::AgentStatus;
    using AgentAssignmentMsg = flychams_api::msg::AgentAssignment;
    using AgentClustersMsg = flychams_api::msg::AgentClusters;
    using ObservationSetpointsMsg = flychams_api::msg::ObservationSetpoints;
    // Operator messages
    using MissionMetricsMsg = flychams_api::msg::MissionMetrics;
    using AgentMetricsMsg = flychams_api::msg::AgentMetrics;
    using TargetMetricsMsg = flychams_api::msg::TargetMetrics;
    using ClusterMetricsMsg = flychams_api::msg::ClusterMetrics;

} // namespace flychams::common