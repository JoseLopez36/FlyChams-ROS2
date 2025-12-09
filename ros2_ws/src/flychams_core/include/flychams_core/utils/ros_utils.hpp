#pragma once

// Standard includes
#include <regex>

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/math_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief ROS utilities
     *
     * @details
     * This class contains ROS utilities
     * ════════════════════════════════════════════════════════════════
     */
    class RosUtils
    {
    public:
        // ════════════════════════════════════════════════════════════════════════════
        // TIMERS: Timer utilities
        // ════════════════════════════════════════════════════════════════════════════

        /**
         * @brief Get the current time
         */
        static Time now(NodePtr node);

        /**
         * @brief Create a timer
         */
        static TimerPtr createTimer(NodePtr node, float rate, const std::function<void()>& callback, CallbackGroupPtr callback_group = nullptr);

        /**
         * @brief Create a wall timer (independent of the node clock)
         */
        static TimerPtr createWallTimer(NodePtr node, float rate, const std::function<void()>& callback, CallbackGroupPtr callback_group = nullptr);

        // ════════════════════════════════════════════════════════════════════════════
        // PARAMETERS: Parameter utilities
        // ════════════════════════════════════════════════════════════════════════════

        /**
         * @brief Get a parameter from the parameter server or shutdown the node
         */
        template <typename T>
        static T getParameter(NodePtr node, const std::string& param_name)
        {
            T value;
            if (!node->get_parameter(param_name, value))
            {
                RCLCPP_ERROR(node->get_logger(), "Failed to get parameter '%s'. Shutting down node '%s'", param_name.c_str(), node->get_name());
                throw rclcpp::exceptions::ParameterNotDeclaredException(param_name);
            }
            return value;
        }

        /**
         * @brief Get a parameter from the parameter server or a default value
         */
        template <typename T>
        static T getParameterOr(NodePtr node, const std::string& param_name, const T& default_value)
        {
            return node->get_parameter_or(param_name, default_value);
        }

        // ════════════════════════════════════════════════════════════════════════════
        // SERVICES: Service utilities
        // ════════════════════════════════════════════════════════════════════════════

        /**
         * @brief Send a request to a service and wait for the response
         */
        template<typename T>
        static bool sendRequest(NodePtr node, ClientPtr<T> client, typename T::Request::SharedPtr request, int wait_time_ms = 1000)
        {
            // First, wait for service to be available
            if (!client->wait_for_service(std::chrono::milliseconds(wait_time_ms)))
            {
                RCLCPP_ERROR(node->get_logger(), "Service %s wait timed out", client->get_service_name());
                return false;
            }

            // Send the request and wait for the response
            client->async_send_request(request);
            return true;
        }

        // ════════════════════════════════════════════════════════════════════════════
        // MESSAGES: Message utilities
        // ════════════════════════════════════════════════════════════════════════════

        /**
         * @brief Convert a PointMsg to a Vector3r
         */
        static Vector3r fromMsg(const PointMsg& point);

        /**
         * @brief Convert a Vector3Msg to a Vector3r
         */
        static Vector3r fromMsg(const Vector3Msg& vector);

        /**
         * @brief Convert a QuaternionMsg to a Quaternionr
         */
        static Quaternionr fromMsg(const QuaternionMsg& quat);

        /**
         * @brief Convert a TransformMsg to a Matrix4r
         */
        static Matrix4r fromMsg(const TransformMsg& transform);

        /**
         * @brief Convert a Vector3r to a PointMsg
         */
        static void toMsg(const Vector3r& vector, PointMsg& point);

        /**
         * @brief Convert a Vector3r to a Vector3Msg
         */
        static void toMsg(const Vector3r& vector, Vector3Msg& vec);

        /**
         * @brief Convert a Quaternionr to a QuaternionMsg
         */
        static void toMsg(const Quaternionr& orientation, QuaternionMsg& quat);

        /**
         * @brief Convert a Matrix4r to a TransformMsg
         */
        static void toMsg(const Matrix4r& matrix, TransformMsg& transform);

        /**
         * @brief Convert a Crop to a CropMsg
         */
        static void toMsg(const Crop& crop, CropMsg& crop_msg);

        // ════════════════════════════════════════════════════════════════════════════
        // OTHER: Other utilities
        // ════════════════════════════════════════════════════════════════════════════

        /**
         * @brief Replace a placeholder in a topic name
         */
        static std::string replace(const std::string& topic_name, const std::string& placeholder, const std::string& value);

        /**
         * @brief Create a header
         */
        static HeaderMsg createHeader(NodePtr node, const std::string& frame_id);

        /**
         * @brief Add an element to a set
         */
        static bool addToSet(NodePtr node, std::unordered_set<ID>& set, const ID& id);

        /**
         * @brief Remove an element from a set
         */
        static bool removeFromSet(NodePtr node, std::unordered_set<ID>& set, const ID& id);
    };

} // namespace flychams::core