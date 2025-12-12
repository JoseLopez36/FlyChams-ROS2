#pragma once

// Standard includes
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/config_types.hpp"
#include "flychams_core/types/ros_types.hpp"
#include "flychams_core/utils/math_utils.hpp"
#include "flychams_core/utils/ros_utils.hpp"

namespace flychams::bringup
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Utility class for creating mission.yaml files from
	 *        configuration spreadsheet
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-12-12
	 * ════════════════════════════════════════════════════════════════
	 */
	class MissionSettingsCreator
	{
	public: // Public methods
		/**
		 * @brief Create mission.yaml file from mission configuration
		 * @param config_ptr Mission configuration pointer
		 * @param path Path to output YAML file
		 * @return true if successful, false otherwise
		 */
		static bool createMissionSettings(const core::MissionConfigPtr& config_ptr, const std::string& path);
		
	private: // Static helper methods for YAML creation
		static void writeMissionSection(std::ostringstream& yaml, const core::MissionConfigPtr& config_ptr);
		static void writeEnvironmentSection(std::ostringstream& yaml, const core::MissionConfigPtr& config_ptr);
		static void writeTargetsSection(std::ostringstream& yaml, const core::MissionConfigPtr& config_ptr);
		static void writeAgentsSection(std::ostringstream& yaml, const core::MissionConfigPtr& config_ptr);
		static void writeTrackingSection(std::ostringstream& yaml, const core::TrackingConfig& tracking, const std::string& prefix);
		static void writeMultiCameraSection(std::ostringstream& yaml, const core::MultiCameraConfigPtr& multi_camera, const std::string& prefix);
		static void writeMultiWindowSection(std::ostringstream& yaml, const core::MultiWindowConfigPtr& multi_window, const std::string& prefix);
		static void writeCameraSection(std::ostringstream& yaml, const core::CameraConfig& camera, const std::string& prefix);
		static void writeGimbalSection(std::ostringstream& yaml, const core::GimbalConfig& gimbal, const std::string& prefix);
		static void writeDroneSection(std::ostringstream& yaml, const core::DroneConfig& drone, const std::string& prefix);
	};

} // namespace flychams::bringup