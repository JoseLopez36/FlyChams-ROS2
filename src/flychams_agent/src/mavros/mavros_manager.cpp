#include "flychams_agent/mavros/mavros_manager.hpp"

// Standard includes
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

// Mavros includes
#include <mavros_msgs/srv/message_interval.hpp>

using namespace flychams::core;

namespace flychams::agent
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void MavrosManager::onInit()
	{
		// Get mavros parameters
		fcu_url_ = RosUtils::getParameterOr<std::string>(node_, "fcu_url", "udp://:14030@172.17.0.2:14280");
		tgt_system_ = RosUtils::getParameterOr<int>(node_, "tgt_system", 1);
		local_position_odom_rate_ = RosUtils::getParameterOr<float>(node_, "local_position_odom_rate", 50.0f);

		// Create service client for setting message intervals
		std::string service_name = "/mavros/" + agent_id_ + "/set_message_interval";
		set_message_interval_client_ = node_->create_client<mavros_msgs::srv::MessageInterval>(service_name);
		RCLCPP_INFO(node_->get_logger(), "Created service client for: %s (agent: %s)", service_name.c_str(), agent_id_.c_str());

		// Launch mavros
		launchMavros();

		// Schedule stream rate configuration after a delay to allow mavros to start
		RCLCPP_INFO(node_->get_logger(), "Creating stream rate configuration timer for agent %s", agent_id_.c_str());
		configure_stream_rate_timer_ = node_->create_wall_timer(std::chrono::duration<float>(0.5f),
			std::bind(&MavrosManager::configureStreamRates, this), 
			module_cb_group_);
	}

	void MavrosManager::onShutdown()
	{
		shutdownMavros();
	}

	// ════════════════════════════════════════════════════════════════════════════
	// METHODS
	// ════════════════════════════════════════════════════════════════════════════

	void MavrosManager::launchMavros()
	{
		// Build the launch command
		std::stringstream cmd;
		cmd << "ros2 launch flychams_agent mavros.launch.py agent_id:=" << agent_id_ << " fcu_url:=" << fcu_url_ << " tgt_system:=" << tgt_system_;

		// Fork a new process to launch mavros
		pid_t pid = fork();
		if (pid == 0)
		{
			// Execute the command
			execl("/bin/sh", "sh", "-c", cmd.str().c_str(), (char*)nullptr);
			exit(1);
		}
		else if (pid > 0)
		{
			// Parent process: store the PID
			mavros_pid_ = pid;
			RCLCPP_INFO(node_->get_logger(), "Launched mavros for agent %s (PID: %d)", agent_id_.c_str(), pid);
		}
		else
		{
			// Fork failed
			RCLCPP_ERROR(node_->get_logger(), "Failed to fork process for launching mavros for agent %s", agent_id_.c_str());
		}
	}

	void MavrosManager::shutdownMavros()
	{
		// Cancel timer if active
		if (configure_stream_rate_timer_)
		{
			configure_stream_rate_timer_->cancel();
			configure_stream_rate_timer_.reset();
		}

		// Reset service client
		set_message_interval_client_.reset();

		if (mavros_pid_ > 0)
		{
			RCLCPP_INFO(node_->get_logger(), "Stopping mavros for agent %s (PID: %d)", agent_id_.c_str(), mavros_pid_);

			// Check if process is still running
			if (kill(mavros_pid_, 0) == 0)
			{
				// Process is still running, send SIGTERM
				kill(mavros_pid_, SIGTERM);
				// Wait for process to terminate
				int status;
				waitpid(mavros_pid_, &status, 0);
			}
			else
			{
				RCLCPP_WARN(node_->get_logger(), "Mavros process for agent %s (PID: %d) is not running", agent_id_.c_str(), mavros_pid_);
			}

			// Reset PID
			mavros_pid_ = 0;
		}
	}

	void MavrosManager::configureStreamRates()
	{
		RCLCPP_INFO(node_->get_logger(), "Attempting to configure stream rates for agent %s", agent_id_.c_str());

		// Check if service is available
		if (!set_message_interval_client_->wait_for_service(std::chrono::milliseconds(1000)))
		{
			// Service not available yet, will retry on next timer callback
			RCLCPP_INFO(node_->get_logger(), "Service '%s' not available yet for agent %s, will retry",
				set_message_interval_client_->get_service_name(), agent_id_.c_str());
			return;
		}

		// Service is available, cancel the timer as we only need to configure once
		if (configure_stream_rate_timer_)
		{
			configure_stream_rate_timer_->cancel();
			configure_stream_rate_timer_.reset();
		}

		// Create request to set LOCAL_POSITION_NED message interval
		// Message ID 32 corresponds to LOCAL_POSITION_NED in MAVLink
		auto request = std::make_shared<mavros_msgs::srv::MessageInterval::Request>();
		request->message_id = 32;  // LOCAL_POSITION_NED
		request->message_rate = local_position_odom_rate_;  // Rate in Hz

		// Log the configuration attempt
		RCLCPP_INFO(node_->get_logger(),
			"Configuring local position odometry stream rate to %.1f Hz for agent %s",
			local_position_odom_rate_, agent_id_.c_str());

		// Send the request asynchronously with a callback
		auto response_received_callback = [this](rclcpp::Client<mavros_msgs::srv::MessageInterval>::SharedFuture future) {
			try
			{
				auto response = future.get();
				if (response->success)
				{
					RCLCPP_INFO(node_->get_logger(),
						"Successfully configured local position odometry stream rate to %.1f Hz for agent %s",
						local_position_odom_rate_, agent_id_.c_str());
				}
				else
				{
					RCLCPP_ERROR(node_->get_logger(),
						"Failed to configure local position odometry stream rate for agent %s. Service returned failure.",
						agent_id_.c_str());
				}
			}
			catch (const std::exception& e)
			{
				RCLCPP_ERROR(node_->get_logger(),
					"Exception while configuring stream rate for agent %s: %s",
					agent_id_.c_str(), e.what());
			}
			};

		set_message_interval_client_->async_send_request(request, response_received_callback);
	}

} // namespace flychams::agent