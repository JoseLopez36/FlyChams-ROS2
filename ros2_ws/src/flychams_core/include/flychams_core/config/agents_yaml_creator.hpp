#pragma once

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/config_types.hpp"
#include "flychams_core/utils/math_utils.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Agents YAML creator for container/mission launch
     *
     * @details
     * This class is responsible for creating agents.yaml file from the
     * given mission configuration. This file contains agent information
     * needed for Docker container launch and mission management.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-12-01
     * ════════════════════════════════════════════════════════════════
     */
    class AgentsYamlCreator
    {
    public: // Public methods
        static bool createAgentsYaml(const MissionConfigPtr& config_ptr, const std::string& path)
        {
            std::ostringstream yaml_content;

            // Write YAML header comment
            yaml_content << "# Agents configuration for mission launch\n";
            yaml_content << "# Generated automatically from configuration spreadsheet\n";
            yaml_content << "# Mission ID: " << config_ptr->id << "\n";
            yaml_content << "# Mission Name: " << config_ptr->name << "\n";
            yaml_content << "# Agent Team ID: " << config_ptr->agent_team_id << "\n\n";

            // Write agents list
            yaml_content << "agents:\n";

            // Iterate through agent team
            for (const auto& [agent_id, agent_config] : config_ptr->agent_team)
            {
                yaml_content << "  - id: " << agent_config->id << "\n";
                yaml_content << "    name: \"" << agent_config->name << "\"\n";
                yaml_content << "    agent_team_id: " << agent_config->agent_team_id << "\n";
                yaml_content << "    tracking_id: " << agent_config->tracking_id << "\n";
                yaml_content << "    drone_id: " << agent_config->drone_id << "\n";

                // Position
                yaml_content << "    position:\n";
                yaml_content << "      x: " << std::fixed << std::setprecision(3)
                    << agent_config->position.x() << "\n";
                yaml_content << "      y: " << agent_config->position.y() << "\n";
                yaml_content << "      z: " << agent_config->position.z() << "\n";

                // Orientation (in degrees for readability)
                yaml_content << "    orientation:\n";
                yaml_content << "      roll: " << std::fixed << std::setprecision(3)
                    << MathUtils::radToDeg(agent_config->orientation.x()) << "\n";
                yaml_content << "      pitch: " << MathUtils::radToDeg(agent_config->orientation.y()) << "\n";
                yaml_content << "      yaw: " << MathUtils::radToDeg(agent_config->orientation.z()) << "\n";

                // Other properties
                yaml_content << "    max_altitude: " << std::fixed << std::setprecision(2)
                    << agent_config->max_altitude << "\n";
                yaml_content << "    safety_radius: " << agent_config->safety_radius << "\n";
                yaml_content << "    battery_capacity: " << agent_config->battery_capacity << "\n";

                yaml_content << "\n";
            }

            // Write summary
            yaml_content << "# Summary\n";
            yaml_content << "total_agents: " << config_ptr->agent_team.size() << "\n";

            // Write to file
            std::ofstream file(path);
            if (file.is_open())
            {
                file << yaml_content.str();
                file.close();
                return true;
            }
            else
            {
                std::cerr << "Error writing agents.yaml to file " << path << std::endl;
                return false;
            }
        }
    };
}

