#pragma once

// Tools includes
#include "flychams_core/config/config_tools.hpp"
#include "flychams_core/ros/transform_tools.hpp"

// ROS includes
#include <mavros_msgs/srv/command_bool.hpp>
#include <mavros_msgs/srv/command_tol.hpp>
#include <mavros_msgs/srv/set_mode.hpp>

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief MavROS interface for handling communication
     *
     * @details
     * This class provides utilities for managing the communication
     * with MavROS.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-01
     * ════════════════════════════════════════════════════════════════
     */
    class MavrosCommunication
    {
    public: // Constructors/Destructors
        MavrosCommunication(const core::ID& agent_id, NodePtr node, const ConfigTools::SharedPtr& config_tools, TransformTools::SharedPtr transform_tools);
        virtual ~MavrosCommunication();
        virtual void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<MavrosCommunication>;

    public: // Vehicle control methods
        bool armDisarm(const bool& arm);
        bool takeoff(const float& z);
        bool land();
        bool enableOffboard(const bool& enable);
        void setPosition(const float& x, const float& y, const float& z);

    private: // Parameters
        core::ID agent_id_;

    private: // Data
        // ROS components
        NodePtr node_;

        // Config tools
        ConfigTools::SharedPtr config_tools_;

        // Transform tools
        TransformTools::SharedPtr transform_tools_;

        // Service clients
        rclcpp::Client<mavros_msgs::srv::CommandBool>::SharedPtr arming_client_;
        rclcpp::Client<mavros_msgs::srv::CommandTOL>::SharedPtr takeoff_client_;
        rclcpp::Client<mavros_msgs::srv::CommandTOL>::SharedPtr land_client_;
        rclcpp::Client<mavros_msgs::srv::SetMode>::SharedPtr set_mode_client_;

        // Publishers
        rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr local_pos_pub_;
    };

} // namespace flychams::core