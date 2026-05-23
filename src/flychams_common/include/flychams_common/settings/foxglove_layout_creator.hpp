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

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Foxglove Studio layout creator
     *
     * @details
     * Generates foxglove/flychams.json from the parsed mission config.
     * The layout is fully agent-agnostic: camera feed tabs, simulation
     * panels, and 3D scene topic subscriptions are all derived from
     * the actual agent IDs and their camera configurations.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-05-23
     * ════════════════════════════════════════════════════════════════
     */
    class FoxgloveLayoutCreator
    {
    public: // Public methods
        /**
         * @brief Create Foxglove Studio layout JSON from mission configuration
         * @param config_ptr Mission configuration pointer
         * @param path Path to output JSON file
         * @return true if successful, false otherwise
         */
        static bool createFoxgloveLayout(const common::MissionConfigPtr& config_ptr, const std::string& path);

    private: // Implementation methods
        static void writeConfigById(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg);
        static void writeMissionButtons(nlohmann::ordered_json& cfg);
        static void writeCameraFeedTabs(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg);
        static void writeImagePanels(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg);
        static void write3DScenePanel(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg);
        static void writeSimulationPanels(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg);
        static void writeOverviewTab(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg);
        static void writeLogPanels(const common::MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg);
        static void writeLayout(nlohmann::ordered_json& root);

    private: // Helper methods
        static std::vector<std::string> getViewIds(const common::AgentConfigPtr& agent_ptr);
    };

} // namespace flychams::common