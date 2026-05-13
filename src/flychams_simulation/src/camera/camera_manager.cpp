#include "flychams_simulation/camera/camera_manager.hpp"

using namespace flychams::core;

namespace flychams::simulation
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void CameraControl::onInit()
    {
        // Get parameters from parameter server
        camera_capture_service_name_ = RosUtils::getParameterOr<std::string>(node_, "camera_capture_service", "/airsim/vehicles/cmd/camera_capture");
        retry_rate_ = RosUtils::getParameterOr<float>(node_, "retry_rate", 1.0f);

        // Initialize data
        agent_cameras_.clear();

        // Set retry timer for pending requests
        retry_timer_ = rclcpp::create_timer(node_, 
            node_->get_clock(),
            std::chrono::duration<float>(1.0f / retry_rate_), 
            std::bind(&CameraControl::retryPendingRequests, this), 
            module_cb_group_);
    }

    void CameraControl::onShutdown()
    {
        // Deactivate all cameras before shutdown
        for (const auto& [agent_id, camera_state] : agent_cameras_)
        {
            if (camera_state.active)
            {
                callCameraCaptureService(agent_id, false);
            }
        }

        // Clear agent camera map
        agent_cameras_.clear();

        // Destroy retry timer
        retry_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // PUBLIC METHODS: Public methods for managing agent cameras
    // ════════════════════════════════════════════════════════════════════════════

    void CameraControl::addAgent(const ID& agent_id)
    {
        // Create service client for this agent
        std::string service_name = camera_capture_service_name_;
        auto client = node_->create_client<airsim_interfaces::srv::CameraCapture>(service_name);

        // Create and add agent camera state
        AgentCameraState camera_state;
        camera_state.active = false;
        camera_state.client = client;
        camera_state.request_pending = false;
        agent_cameras_.insert({ agent_id, std::move(camera_state) });

        RCLCPP_INFO(node_->get_logger(), "Camera control: Added agent %s", agent_id.c_str());

        // Activate camera for this agent
        setAgentCameraActive(agent_id, true);
    }

    void CameraControl::removeAgent(const ID& agent_id)
    {
        // Check if agent exists
        auto it = agent_cameras_.find(agent_id);
        if (it == agent_cameras_.end())
        {
            RCLCPP_WARN(node_->get_logger(), "Camera control: Cannot remove unknown agent %s", agent_id.c_str());
            return;
        }

        // Deactivate camera before removal
        if (it->second.active)
        {
            callCameraCaptureService(agent_id, false);
        }

        // Remove agent from map
        agent_cameras_.erase(it);

        RCLCPP_INFO(node_->get_logger(), "Camera control: Removed agent %s", agent_id.c_str());
    }

    void CameraControl::setAgentCameraActive(const ID& agent_id, bool active)
    {
        // Check if agent exists
        auto it = agent_cameras_.find(agent_id);
        if (it == agent_cameras_.end())
        {
            RCLCPP_WARN(node_->get_logger(), "Camera control: Cannot set camera state for unknown agent %s", agent_id.c_str());
            return;
        }

        // Check if state actually changed
        if (it->second.active == active)
        {
            return; // No change needed
        }

        // Call service to set camera state
        callCameraCaptureService(agent_id, active);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CONTROL: Control methods
    // ════════════════════════════════════════════════════════════════════════════

    void CameraControl::retryPendingRequests()
    {
        // Check all agents for pending requests and retry if service is available
        for (auto& [agent_id, camera_state] : agent_cameras_)
        {
            if (camera_state.request_pending && camera_state.client->service_is_ready())
            {
                // Resend the last request
                bool desired_state = camera_state.active;
                callCameraCaptureService(agent_id, desired_state);
            }
        }
    }

    void CameraControl::callCameraCaptureService(const ID& agent_id, bool active)
    {
        auto it = agent_cameras_.find(agent_id);
        if (it == agent_cameras_.end())
        {
            return;
        }

        auto& camera_state = it->second;

        // Create request
        auto request = std::make_shared<airsim_interfaces::srv::CameraCapture::Request>();
        request->vehicle_name = agent_id;
        request->active = active;

        // Check if service is ready
        if (!camera_state.client->service_is_ready())
        {
            RCLCPP_DEBUG(node_->get_logger(), "Camera control: Service not ready for agent %s, will retry", agent_id.c_str());
            camera_state.request_pending = true;
            return;
        }

        // Mark request as pending
        camera_state.request_pending = true;

        // Call service asynchronously
        auto future = camera_state.client->async_send_request(request,
            [this, agent_id, active](rclcpp::Client<airsim_interfaces::srv::CameraCapture>::SharedFuture future)
            {
                this->handleServiceResponse(agent_id, active, future);
            });

        RCLCPP_DEBUG(node_->get_logger(), "Camera control: Sending camera %s request for agent %s", 
            active ? "activation" : "deactivation", agent_id.c_str());
    }

    void CameraControl::handleServiceResponse(const ID& agent_id, bool active, rclcpp::Client<airsim_interfaces::srv::CameraCapture>::SharedFuture future)
    {
        auto it = agent_cameras_.find(agent_id);
        if (it == agent_cameras_.end())
        {
            return;
        }

        auto& camera_state = it->second;

        try
        {
            auto response = future.get();
            if (response->success)
            {
                camera_state.active = active;
                camera_state.request_pending = false;
                RCLCPP_INFO(node_->get_logger(), "Camera control: Camera %s successful for agent %s",
                    active ? "activation" : "deactivation", agent_id.c_str());
            }
            else
            {
                RCLCPP_WARN(node_->get_logger(), "Camera control: Camera %s failed for agent %s (service returned false)",
                    active ? "activation" : "deactivation", agent_id.c_str());
                camera_state.request_pending = true; // Will retry on next timer tick
            }
        }
        catch (const std::exception& e)
        {
            RCLCPP_ERROR(node_->get_logger(), "Camera control: Service call failed for agent %s: %s",
                agent_id.c_str(), e.what());
            camera_state.request_pending = true; // Will retry on next timer tick
        }
    }

} // namespace flychams::simulation