#pragma once

// Standard includes
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

// Types includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/config_types.hpp"
#include "flychams_common/types/ros_types.hpp"

// Utils includes
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"

namespace flychams::common
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
		static bool createMissionSettings(const common::MissionConfigPtr& config_ptr, const std::string& path);
		
	private: // Static helper methods for YAML creation
		static void writeMissionSection(std::ostringstream& yaml, const common::MissionConfigPtr& config_ptr);
		static void writeEnvironmentSection(std::ostringstream& yaml, const common::MissionConfigPtr& config_ptr);
		static void writeTargetsSection(std::ostringstream& yaml, const common::MissionConfigPtr& config_ptr);
		static void writeAgentsSection(std::ostringstream& yaml, const common::MissionConfigPtr& config_ptr);
		static void writeTrackingSection(std::ostringstream& yaml, const common::TrackingConfig& tracking, const std::string& prefix);
		static void writeMultiCameraSection(std::ostringstream& yaml, const common::MultiCameraConfigPtr& multi_camera, const std::string& prefix);
		static void writeMultiWindowSection(std::ostringstream& yaml, const common::MultiWindowConfigPtr& multi_window, const std::string& prefix);
		static void writeCameraSection(std::ostringstream& yaml, const common::CameraConfig& camera, const std::string& prefix);
		static void writeGimbalSection(std::ostringstream& yaml, const common::GimbalConfig& gimbal, const std::string& prefix);
		static void writeDroneSection(std::ostringstream& yaml, const common::DroneConfig& drone, const std::string& prefix);
	};

} // namespace flychams::common