#pragma once

// Types includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/config_types.hpp"
#include "flychams_common/types/ros_types.hpp"

// Utils includes
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"

namespace flychams::core
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Utility class for parsing mission parameters 
	 *        from YAML configuration files
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-12-12
	 * ════════════════════════════════════════════════════════════════
	 */
	class MissionSettingsParser
	{
	public: // Static helper methods for mission parameters parsing
	    static void parseMissionParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr);
		static void parseEnvironmentParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr);
		static void parseTargetParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr);
		static void parseAgentParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr);
		static void parseTrackingParameters(const NodePtr& node, AgentConfigPtr& agent, const std::string& prefix);
		static void parseMultiCameraParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix);
		static void parseMultiWindowParameters(const NodePtr& node, MultiWindowConfigPtr& multi_window, const std::string& prefix);
		static void parseCameraParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix);
		static void parseGimbalParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix);
		static void parseDroneParameters(const NodePtr& node, AgentConfigPtr& agent, const std::string& prefix);

	public: // Static helper methods for other parameters parsing
		static void parseSystemParameters(const NodePtr& node, MissionConfigPtr& config_ptr);
		static void parseTopicParameters(const NodePtr& node, MissionConfigPtr& config_ptr);
		static void parseFrameParameters(const NodePtr& node, MissionConfigPtr& config_ptr);
	};

} // namespace flychams::core