#include "flychams_core/utils/ros_utils.hpp"

namespace flychams::core
{
    // ════════════════════════════════════════════════════════════════════════════
    // TIMERS: Timer utilities
    // ════════════════════════════════════════════════════════════════════════════

    Time RosUtils::now(NodePtr node)
    {
        return node->get_clock()->now();
    }

    TimerPtr RosUtils::createTimer(NodePtr node, float rate, const std::function<void()>& callback, CallbackGroupPtr callback_group)
    {
        auto steady_clock = std::make_shared<rclcpp::Clock>(RCL_STEADY_TIME);
        auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::duration<float>(1.0f / rate));
        return rclcpp::create_timer(node.get(), steady_clock, period, callback, callback_group);
    }

    TimerPtr RosUtils::createWallTimer(NodePtr node, float rate, const std::function<void()>& callback, CallbackGroupPtr callback_group)
    {
        if (callback_group == nullptr)
        {
            return node->create_wall_timer(std::chrono::duration<float>(1.0f / rate), callback);
        }
        else
        {
            return node->create_wall_timer(std::chrono::duration<float>(1.0f / rate), callback, callback_group);
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // MESSAGES: Message utilities
    // ════════════════════════════════════════════════════════════════════════════

    Vector3r RosUtils::fromMsg(const PointMsg& point)
    {
        return Vector3r{ static_cast<float>(point.x), static_cast<float>(point.y), static_cast<float>(point.z) };
    }

    Vector3r RosUtils::fromMsg(const Vector3Msg& vector)
    {
        return Vector3r{ static_cast<float>(vector.x), static_cast<float>(vector.y), static_cast<float>(vector.z) };
    }

    Quaternionr RosUtils::fromMsg(const QuaternionMsg& quat)
    {
        return Quaternionr{ static_cast<float>(quat.w), static_cast<float>(quat.x), static_cast<float>(quat.y), static_cast<float>(quat.z) };
    }

    Matrix4r RosUtils::fromMsg(const TransformMsg& transform)
    {
        Matrix4r T = Matrix4r::Identity();
        T.block<3, 1>(0, 3) = fromMsg(transform.translation);
        Quaternionr q = fromMsg(transform.rotation);
        T.block<3, 3>(0, 0) = MathUtils::quatToMatrix(q);
        return T;
    }

    void RosUtils::toMsg(const Vector3r& vector, PointMsg& point)
    {
        point.x = static_cast<double>(vector.x());
        point.y = static_cast<double>(vector.y());
        point.z = static_cast<double>(vector.z());
    }

    void RosUtils::toMsg(const Vector3r& vector, Vector3Msg& vec)
    {
        vec.x = static_cast<double>(vector.x());
        vec.y = static_cast<double>(vector.y());
        vec.z = static_cast<double>(vector.z());
    }

    void RosUtils::toMsg(const Quaternionr& orientation, QuaternionMsg& quat)
    {
        quat.x = static_cast<double>(orientation.x());
        quat.y = static_cast<double>(orientation.y());
        quat.z = static_cast<double>(orientation.z());
        quat.w = static_cast<double>(orientation.w());
    }

    void RosUtils::toMsg(const Matrix4r& matrix, TransformMsg& transform)
    {
        toMsg(matrix.block<3, 1>(0, 3), transform.translation);
        toMsg(MathUtils::quatFromMatrix(matrix.block<3, 3>(0, 0)), transform.rotation);
    }

    void RosUtils::toMsg(const Crop& crop, CropMsg& crop_msg)
    {
        crop_msg.x = crop.x;
        crop_msg.y = crop.y;
        crop_msg.w = crop.w;
        crop_msg.h = crop.h;
        crop_msg.is_out_of_bounds = crop.is_out_of_bounds;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // OTHER: Other utilities
    // ════════════════════════════════════════════════════════════════════════════

    std::string RosUtils::replace(const std::string& topic_name, const std::string& placeholder, const std::string& value)
    {
        return std::regex_replace(topic_name, std::regex(placeholder), value);
    }

    HeaderMsg RosUtils::createHeader(NodePtr node, const std::string& frame_id)
    {
        HeaderMsg header;
        header.frame_id = frame_id;
        header.stamp = now(node);
        return header;
    }

    bool RosUtils::addToSet(NodePtr node, std::unordered_set<ID>& set, const ID& id)
    {
        // Check if element already exists
        if (set.find(id) != set.end())
        {
            RCLCPP_INFO(node->get_logger(), "Element %s already exists. Skipping addition", id.c_str());
            return false;
        }
        // Insert element
        set.insert(id);
        return true;
    }

    bool RosUtils::removeFromSet(NodePtr node, std::unordered_set<ID>& set, const ID& id)
    {
        // Check if element exists
        if (set.find(id) == set.end())
        {
            RCLCPP_INFO(node->get_logger(), "Element %s does not exist. Skipping removal", id.c_str());
            return false;
        }
        // Remove element
        set.erase(id);
        return true;
    }

} // namespace flychams::core