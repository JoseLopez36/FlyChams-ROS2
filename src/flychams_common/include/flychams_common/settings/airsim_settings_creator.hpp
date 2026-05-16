#pragma once

/* JSON Files Management: https://github.com/nlohmann/json */
#include <nlohmann/json.hpp>

// Standard includes
#include <fstream>
#include <iostream>
#include <string>

// Core includes
#include "flychams_common/types/core_types.hpp"
#include "flychams_common/types/config_types.hpp"
#include "flychams_common/utils/math_utils.hpp"
#include "flychams_common/utils/vision_utils.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief AirSim settings creator
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-03-25
     * ════════════════════════════════════════════════════════════════
     */
    class AirsimSettingsCreator
    {
    public: // Public methods
        /**
		 * @brief Create AirSim settings.json file from mission configuration
		 * @param config_ptr Mission configuration pointer
		 * @param path Path to output JSON file
		 * @return true if successful, false otherwise
		 */
        static bool createAirsimSettings(const common::MissionConfigPtr& config_ptr, const std::string& path);

    private: // Implementation methods
        static void writeGeneralSection(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& settings);
        static void writeQualitySettingsSection(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& quality_settings);
        static void writeVehiclesSection(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& vehicles);
        static void writeSensorsSection(const common::ID& agent_id, const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& sensors);
        static void writeInternalCamerasSection(const common::ID& agent_id, const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cameras);
        static void writeExternalCamerasSection(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cameras);
        static void writeSubWindowsSection(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& subwindows);
        static void writeStreamsSection(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& streams);
    };

} // namespace flychams::common