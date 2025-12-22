#pragma once

/* JSON Files Management: https://github.com/nlohmann/json */
#include <nlohmann/json.hpp>

// Standard includes
#include <fstream>
#include <iostream>
#include <string>

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/config_types.hpp"
#include "flychams_core/utils/math_utils.hpp"
#include "flychams_core/utils/vision_utils.hpp"

namespace flychams::core
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
        static bool createAirsimSettings(const core::MissionConfigPtr& config_ptr, const std::string& path);

    private: // Implementation methods
        static void writeGeneralSection(const core::MissionConfigPtr& config_ptr, nlohmann::ordered_json& settings);
        static void writeVehiclesSection(const core::MissionConfigPtr& config_ptr, nlohmann::ordered_json& vehicles);
        static void writeSensorsSection(const core::ID& agent_id, const core::MissionConfigPtr& config_ptr, nlohmann::ordered_json& sensors);
        static void writeInternalCamerasSection(const core::ID& agent_id, const core::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cameras);
        static void writeExternalCamerasSection(const core::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cameras);
        static void writeSubWindowsSection(const core::MissionConfigPtr& config_ptr, nlohmann::ordered_json& subwindows);
        static void writeStreamsSection(const core::MissionConfigPtr& config_ptr, nlohmann::ordered_json& streams);
    };

} // namespace flychams::core