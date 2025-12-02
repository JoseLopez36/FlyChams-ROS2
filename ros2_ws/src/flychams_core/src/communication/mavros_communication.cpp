#include "flychams_core/communication/mavros_communication.hpp"

namespace flychams::core
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    MavrosCommunication::MavrosCommunication(const core::ID& agent_id, NodePtr node, const ConfigTools::SharedPtr& config_tools, TransformTools::SharedPtr transform_tools)
        : node_(node), config_tools_(config_tools), transform_tools_(transform_tools), agent_id_(agent_id)
    {
        // Initialize ROS components
        arming_client_ = node_->create_client<mavros_msgs::srv::CommandBool>("/mavros/" + agent_id + "/cmd/arming");
        takeoff_client_ = node_->create_client<mavros_msgs::srv::CommandTOL>("/mavros/" + agent_id + "/cmd/takeoff");
        land_client_ = node_->create_client<mavros_msgs::srv::CommandTOL>("/mavros/" + agent_id + "/cmd/land");
        set_mode_client_ = node_->create_client<mavros_msgs::srv::SetMode>("/mavros/" + agent_id + "/set_mode");
        local_pos_pub_ = node_->create_publisher<geometry_msgs::msg::PoseStamped>("/mavros/" + agent_id + "/setpoint_position/local", 10);
    }

    MavrosCommunication::~MavrosCommunication()
    {
        shutdown();
    }

    void MavrosCommunication::shutdown()
    {
        // Destroy clients
        arming_client_.reset();
        takeoff_client_.reset();
        land_client_.reset();
        set_mode_client_.reset();
        // Destroy publishers
        local_pos_pub_.reset();
        // Destroy config tools
        config_tools_.reset();
        // Destroy node pointer
        node_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // VEHICLE CONTROL
    // ════════════════════════════════════════════════════════════════════════════

    bool MavrosCommunication::armDisarm(const bool& arm)
    {
        auto request = std::make_shared<mavros_msgs::srv::CommandBool::Request>();
        request->value = arm;

        return RosUtils::sendRequest<mavros_msgs::srv::CommandBool>(node_, arming_client_, request, 2000);
    }

    bool MavrosCommunication::takeoff(const float& z)
    {
        auto request = std::make_shared<mavros_msgs::srv::CommandTOL::Request>();
        request->altitude = z;

        return RosUtils::sendRequest<mavros_msgs::srv::CommandTOL>(node_, takeoff_client_, request, 2000);
    }

    bool MavrosCommunication::land()
    {
        auto request = std::make_shared<mavros_msgs::srv::CommandTOL::Request>();

        return RosUtils::sendRequest<mavros_msgs::srv::CommandTOL>(node_, land_client_, request, 2000);
    }

    bool MavrosCommunication::enableOffboard(const bool& enable)
    {
        // Switch to OFFBOARD mode
        auto request = std::make_shared<mavros_msgs::srv::SetMode::Request>();
        request->custom_mode = enable ? "OFFBOARD" : "AUTO.LOITER";

        return RosUtils::sendRequest<mavros_msgs::srv::SetMode>(node_, set_mode_client_, request, 2000);
    }

    void MavrosCommunication::setPosition(const float& x, const float& y, const float& z)
    {
        geometry_msgs::msg::PoseStamped msg;
        msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        msg.pose.position.x = x;
        msg.pose.position.y = y;
        msg.pose.position.z = z;
        msg.pose.orientation.w = 1.0;

        local_pos_pub_->publish(msg);
    }

} // namespace flychams::core
