#include "flychams_agent/mavros/mavros_communication.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void MavrosCommunication::onModuleInit()
    {
        // Initialize ROS components
        arming_client_ = node_->create_client<mavros_msgs::srv::CommandBool>("/mavros/" + agent_id_ + "/cmd/arming");
        takeoff_client_ = node_->create_client<mavros_msgs::srv::CommandTOL>("/mavros/" + agent_id_ + "/cmd/takeoff");
        land_client_ = node_->create_client<mavros_msgs::srv::CommandTOL>("/mavros/" + agent_id_ + "/cmd/land");
        set_mode_client_ = node_->create_client<mavros_msgs::srv::SetMode>("/mavros/" + agent_id_ + "/set_mode");
        local_pos_pub_ = node_->create_publisher<PoseStampedMsg>("/mavros/" + agent_id_ + "/setpoint_position/local", 10);
    }

    void MavrosCommunication::onModuleShutdown()
    {
        // Destroy clients
        arming_client_.reset();
        takeoff_client_.reset();
        land_client_.reset();
        set_mode_client_.reset();
        // Destroy publishers
        local_pos_pub_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // VEHICLE STATE
    // ════════════════════════════════════════════════════════════════════════════

    SubscriberPtr<mavros_msgs::msg::HomePosition> MavrosCommunication::subscribeHomePosition(const std::function<void(const mavros_msgs::msg::HomePosition::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options)
    {
        rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();
        return node_->create_subscription<mavros_msgs::msg::HomePosition>("/mavros/" + agent_id_ + "/home_position/home", qos, callback, options);
    }

    SubscriberPtr<mavros_msgs::msg::State> MavrosCommunication::subscribeState(const std::function<void(const mavros_msgs::msg::State::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options)
    {
        rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();
        return node_->create_subscription<mavros_msgs::msg::State>("/mavros/" + agent_id_ + "/state", qos, callback, options);
    }

    SubscriberPtr<OdometryMsg> MavrosCommunication::subscribeLocalOdometry(const std::function<void(const OdometryMsg::SharedPtr)>& callback, const rclcpp::SubscriptionOptions& options)
    {
        rclcpp::QoS qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();
        return node_->create_subscription<OdometryMsg>("/mavros/" + agent_id_ + "/local_position/odom", qos, callback, options);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // VEHICLE CONTROL
    // ════════════════════════════════════════════════════════════════════════════

    bool MavrosCommunication::armDisarm(const bool& arm)
    {
        auto request = std::make_shared<mavros_msgs::srv::CommandBool::Request>();
        request->value = arm;

        return node_->sendRequest<mavros_msgs::srv::CommandBool>(arming_client_, request, 2000);
    }

    bool MavrosCommunication::takeoff(const float& z)
    {
        auto request = std::make_shared<mavros_msgs::srv::CommandTOL::Request>();
        request->altitude = z;

        return node_->sendRequest<mavros_msgs::srv::CommandTOL>(takeoff_client_, request, 2000);
    }

    bool MavrosCommunication::land()
    {
        auto request = std::make_shared<mavros_msgs::srv::CommandTOL::Request>();

        return node_->sendRequest<mavros_msgs::srv::CommandTOL>(land_client_, request, 2000);
    }

    bool MavrosCommunication::enableOffboard(const bool& enable)
    {
        // Switch to OFFBOARD mode
        auto request = std::make_shared<mavros_msgs::srv::SetMode::Request>();
        request->custom_mode = enable ? "OFFBOARD" : "AUTO.LOITER";

        return node_->sendRequest<mavros_msgs::srv::SetMode>(set_mode_client_, request, 2000);
    }

    void MavrosCommunication::setLocalPosition(const float& x, const float& y, const float& z)
    {
        // Create local pose stamped message
        PoseStampedMsg msg;
        msg.header = node_->createHeader(node_->getAgentLocalFrame(agent_id_));
        msg.pose.position.x = x;
        msg.pose.position.y = y;
        msg.pose.position.z = z;
        msg.pose.orientation.w = 1.0;

        // Publish message
        local_pos_pub_->publish(msg);
    }

} // namespace flychams::agent
