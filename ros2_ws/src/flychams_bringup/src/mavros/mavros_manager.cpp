#include "flychams_bringup/mavros/mavros_manager.hpp"

// Standard includes
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

using namespace flychams::core;

namespace flychams::bringup
{
	// ════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR: Constructor and destructor
	// ════════════════════════════════════════════════════════════════════════════

	void MavrosManager::onInit()
	{
		// Get mavros parameters
		fcu_url_ = RosUtils::getParameterOr<std::string>(node_, "fcu_url", "udp://:14030@172.17.0.2:14280");
		tgt_system_ = RosUtils::getParameterOr<int>(node_, "tgt_system", 1);

		// Launch mavros
		launchMavros();
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
		cmd << "ros2 launch flychams_bringup run_mavros.launch.py agent_id:=" << agent_id_ << " fcu_url:=" << fcu_url_ << " tgt_system:=" << tgt_system_;

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

} // namespace flychams::bringup