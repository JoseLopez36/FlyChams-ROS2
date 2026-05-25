#include "flychams_common/settings/foxglove_layout_creator.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// JSON CREATION
// ════════════════════════════════════════════════════════════════════════════

bool FoxgloveLayoutCreator::createFoxgloveLayout(const MissionConfigPtr& config_ptr, const std::string& path)
{
    nlohmann::ordered_json root;
    writeConfigById(config_ptr, root["configById"]);
    root["globalVariables"] = nlohmann::ordered_json::object();
    root["userNodes"] = nlohmann::ordered_json::object();
    root["playbackConfig"] = { {"speed", 1} };
    writeLayout(root);

    std::ofstream file(path);
    if (file.is_open())
    {
        file << root.dump(2);
        file.close();
    }
    else
    {
        std::cerr << "Error writing Foxglove layout to file " << path << std::endl;
        return false;
    }

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// CONFIG BY ID
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::writeConfigById(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg)
{
    writeMissionButtons(cfg);
    writeCameraFeedTabs(config_ptr, cfg);
    writeImagePanels(config_ptr, cfg);
    write3DScenePanel(config_ptr, cfg);
    writeOverviewTab(config_ptr, cfg);
    writeSimulationPanels(config_ptr, cfg);
    writeLogPanels(config_ptr, cfg);

    // Mission / fleet button groups
    cfg["Group!mission"] = {
        {"first", {
            {"first", "button.Button!mstart"},
            {"second", "button.Button!mpause"},
            {"direction", "row"}
        }},
        {"second", "button.Button!mabort"},
        {"direction", "column"},
        {"splitPercentage", 50}
    };
    cfg["Group!fleet"] = {
        {"first", {
            {"first", "button.Button!armall"},
            {"second", "button.Button!landall"},
            {"direction", "row"}
        }},
        {"second", "button.Button!rth"},
        {"direction", "column"},
        {"splitPercentage", 50}
    };
}

void FoxgloveLayoutCreator::writeMissionButtons(nlohmann::ordered_json& cfg)
{
    cfg["button.Button!mstart"] = {
        {"outputMode", "publisher"},
        {"name", "/flychams/operator/start_mission"},
        {"buttonMode", "toggle"},
        {"activeColor", "#34d399"},
        {"inactiveColor", "#10b981"},
        {"activeText", "Starting"},
        {"inactiveText", "Start"},
        {"foxglovePanelTitle", "Mission: Start"}
    };
    cfg["button.Button!mpause"] = {
        {"outputMode", "publisher"},
        {"name", "/flychams/operator/pause_mission"},
        {"buttonMode", "toggle"},
        {"activeColor", "#f59e0b"},
        {"inactiveColor", "#D97706"},
        {"activeText", "Paused"},
        {"inactiveText", "Pause"},
        {"foxglovePanelTitle", "Mission: Pause"}
    };
    cfg["button.Button!mabort"] = {
        {"outputMode", "publisher"},
        {"name", "/flychams/operator/abort_mission"},
        {"buttonMode", "toggle"},
        {"activeColor", "#f87171"},
        {"inactiveColor", "#DC2626"},
        {"activeText", "Aborting"},
        {"inactiveText", "Abort"},
        {"foxglovePanelTitle", "Mission: Abort"}
    };
    cfg["button.Button!armall"] = {
        {"outputMode", "publisher"},
        {"name", "/flychams/operator/arm_all"},
        {"buttonMode", "toggle"},
        {"activeColor", "#059669"},
        {"inactiveColor", "#DC2626"},
        {"activeText", "Armed"},
        {"inactiveText", "Disarmed"},
        {"foxglovePanelTitle", "Fleet: Arm"}
    };
    cfg["button.Button!landall"] = {
        {"outputMode", "publisher"},
        {"name", "/flychams/operator/land_all"},
        {"buttonMode", "toggle"},
        {"activeColor", "#fbbf24"},
        {"inactiveColor", "#d97706"},
        {"activeText", "Landing"},
        {"inactiveText", "Land All"},
        {"foxglovePanelTitle", "Fleet: Land"}
    };
    cfg["button.Button!rth"] = {
        {"outputMode", "publisher"},
        {"name", "/flychams/operator/return_home"},
        {"buttonMode", "toggle"},
        {"activeColor", "#22d3ee"},
        {"inactiveColor", "#0891B2"},
        {"activeText", "Returning"},
        {"inactiveText", "Home All"},
        {"foxglovePanelTitle", "Fleet: Home"}
    };
}

// ════════════════════════════════════════════════════════════════════════════
// CAMERA FEED TABS
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::writeCameraFeedTabs(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg)
{
    nlohmann::ordered_json tabs = nlohmann::ordered_json::array();

    int tab_idx = 0;
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        char tab_buf[8];
        std::snprintf(tab_buf, sizeof(tab_buf), "tab%02d", tab_idx);
        const std::string tab(tab_buf);

        const auto views = getViewIds(agent_ptr);
        const int nv = static_cast<int>(views.size());

        // Build the panel ID list for this tab
        // Layout: central (view00) on top-left; tracking views in a 2x2 grid on the right
        const std::string v00 = "Image!" + tab + "view00";
        const std::string v01 = "Image!" + tab + "view01";
        const std::string v02 = "Image!" + tab + "view02";
        const std::string v03 = "Image!" + tab + "view03";
        const std::string v04 = "Image!" + tab + "view04";

        tabs.push_back({
            {"title", "ID: " + agent_id + " Feed"},
            {"layout", {
                {"first", v00},
                {"second", {
                    {"first", {
                        {"first", v01},
                        {"second", v02},
                        {"direction", "row"}
                    }},
                    {"second", {
                        {"first", v03},
                        {"second", v04},
                        {"direction", "row"}
                    }},
                    {"direction", "column"}
                }},
                {"direction", "column"},
                {"splitPercentage", 50}
            }}
        });

        ++tab_idx;
    }

    cfg["Tab!cameras"] = {
        {"activeTabIdx", 0},
        {"tabs", tabs}
    };
}

// ════════════════════════════════════════════════════════════════════════════
// IMAGE PANELS
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::writeImagePanels(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg)
{
    int tab_idx = 0;
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        char tab_buf[8];
        std::snprintf(tab_buf, sizeof(tab_buf), "tab%02d", tab_idx);
        const std::string tab(tab_buf);
        const auto views = getViewIds(agent_ptr);

        const int nv = static_cast<int>(views.size());
        for (int vi = 0; vi < 5; ++vi)
        {
            char view_buf[8];
            std::snprintf(view_buf, sizeof(view_buf), "view%02d", vi);
            const std::string panel_id = "Image!" + tab + view_buf;

            if (vi < nv)
            {
                const std::string agent_base = "/flychams/agent/" + agent_id + "/" + views[vi];
                const std::string op_base    = "/flychams/operator/" + agent_id + "/" + views[vi];
                cfg[panel_id] = {
                    {"synchronize", false},
                    {"imageMode", {
                        {"imageTopic", agent_base + "/image/compressed"},
                        {"annotations", {
                            {op_base + "/annotations", { {"visible", true} }}
                        }}
                    }},
                    {"foxglovePanelTitle", "ID: " + views[vi]}
                };
            }
            else
            {
                cfg[panel_id] = {
                    {"synchronize", false},
                    {"imageMode", {
                        {"imageTopic", ""},
                        {"annotations", {
                            {"", { {"visible", false} }}
                        }}
                    }},
                    {"foxglovePanelTitle", ""}
                };
            }
        }

        ++tab_idx;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// 3D SCENE PANEL
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::write3DScenePanel(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg)
{
    // Agent palette hex colors — must match AgentColors::kPalette order in color_dictionary.hpp
    static const char* kAgentHex[] = {
        "#00d9ff",  // 0 kCyan
        "#33cc66",  // 1 kLime
        "#ffc700",  // 2 kAmber        
        "#8c33ff",  // 3 kViolet
        "#ff9966",  // 4 kPeach
        "#008cff",  // 5 kSkyBlue
        "#00cca6",  // 6 kTeal
        "#d900d9",  // 7 kMagenta
    };
    static constexpr int kPaletteSize = 8;

    // Build camera_info topic subscriptions; assign per-agent frustum color
    nlohmann::ordered_json topics;
    topics["/flychams/operator/scene"] = { {"visible", true} };

    int agent_idx = 0;
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        const std::string hex = kAgentHex[agent_idx % kPaletteSize];
        // Only cameras have a physical frustum
        for (const auto& [camera_id, camera_ptr] : agent_ptr->tracking.multi_camera_set)
        {
            const std::string topic = "/flychams/agent/" + agent_id + "/" + camera_id + "/camera_info";
            topics[topic] = {
                {"visible", true},
                {"distance", 6},
                {"planarProjectionFactor", 0.5},
                {"width", 0.03},
                {"color", hex + "cc"}
            };
        }
        ++agent_idx;
    }

    cfg["3D!scene"] = {
        {"cameraState", {
            {"perspective", true},
            {"distance", 120},
            {"phi", 35},
            {"thetaOffset", 50},
            {"targetOffset", {0, 0, 0}},
            {"target", {0, 0, 0}},
            {"targetOrientation", {0, 0, 0, 1}},
            {"fovy", 55},
            {"near", 0.1},
            {"far", 5000}
        }},
        {"followMode", "follow-none"},
        {"scene", {
            {"enableStats", false},
            {"backgroundColor", "#0d1117"},
            {"labelScaleFactor", 0.8},
            {"ignoreColladaUpAxis", false},
            {"transforms", {
                {"visible", false},
                {"enablePreloading", true},
                {"labelSize", 1.5},
                {"axisSize", 6},
                {"lineWidth", 2}
            }}
        }},
        {"transforms", {
            {"world", {
                {"visible", true},
                {"enablePreloading", true},
                {"xAxisColor", "#ff4444"},
                {"yAxisColor", "#44ff44"},
                {"zAxisColor", "#4444ff"},
                {"lineWidth", 0}
            }}
        }},
        {"topics", topics},
        {"layers", {
            {"grid", {
                {"visible", true},
                {"drawBehind", true},
                {"instanceId", "flychams"},
                {"layerId", "foxglove.Grid"},
                {"size", 500},
                {"divisions", 50},
                {"lineWidth", 1.5},
                {"color", "#ffffff1a"},
                {"position", {0, 0, 0}},
                {"rotation", {0, 0, 0}}
            }}
        }},
        {"publish", {
            {"type", "pose"},
            {"poseTopic", "/move_base_simple/goal"},
            {"pointTopic", "/clicked_point"},
            {"poseEstimateTopic", "/initialpose"},
            {"poseEstimateXDeviation", 0.5},
            {"poseEstimateYDeviation", 0.5},
            {"poseEstimateThetaDeviation", 0.26179939}
        }},
        {"synchronize", false},
        {"imageMode", nlohmann::ordered_json::object()},
        {"foxglovePanelTitle", "Situational Awareness"},
        {"fixedFrame", "world"},
        {"followTf", "world"}
    };
}

// ════════════════════════════════════════════════════════════════════════════
// SIMULATION PANELS
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::writeSimulationPanels(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg)
{
    // Scenario view (single)
    cfg["Image!simscenario"] = {
        {"synchronize", false},
        {"imageMode", {
            {"imageTopic", "/flychams/simulation/scenario/image/compressed"}
        }},
        {"foxglovePanelTitle", "Scenario"}
    };

    // Per-agent agent_view and payload_view panels
    int i = 0;
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        char suf_buf[4];
        std::snprintf(suf_buf, sizeof(suf_buf), "%02d", i);
        const std::string suffix(suf_buf);

        cfg["Image!simagent" + suffix] = {
            {"synchronize", false},
            {"imageMode", {
                {"imageTopic", "/flychams/simulation/" + agent_id + "/body/image/compressed"}
            }},
            {"foxglovePanelTitle", "ID: " + agent_id + " Agent View"}
        };
        cfg["Image!simpayload" + suffix] = {
            {"synchronize", false},
            {"imageMode", {
                {"imageTopic", "/flychams/simulation/" + agent_id + "/payload/image/compressed"}
            }},
            {"foxglovePanelTitle", "ID: " + agent_id + " Payload View"}
        };
        ++i;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// OVERVIEW TAB
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::writeOverviewTab(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg)
{
    nlohmann::ordered_json tabs = nlohmann::ordered_json::array();

    // 3D Scene tab
    tabs.push_back({ {"title", "3D Scene"}, {"layout", "3D!scene"} });

    // One simulation tab per agent
    int i = 0;
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        char suf_buf[4];
        std::snprintf(suf_buf, sizeof(suf_buf), "%02d", i);
        const std::string suffix(suf_buf);
        tabs.push_back({
            {"title", "ID: " + agent_id + " Simulation Feed"},
            {"layout", {
                {"first", "Image!simscenario"},
                {"second", {
                    {"first", "Image!simagent" + suffix},
                    {"second", "Image!simpayload" + suffix},
                    {"direction", "row"},
                    {"splitPercentage", 50}
                }},
                {"direction", "column"},
                {"splitPercentage", 66.66}
            }}
        });
        ++i;
    }

    cfg["Tab!overview"] = {
        {"activeTabIdx", 0},
        {"tabs", tabs}
    };
}

// ════════════════════════════════════════════════════════════════════════════
// LOG PANELS
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::writeLogPanels(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cfg)
{
    // System-wide log
    cfg["RosOut!log"] = {
        {"minLogLevel", 1},
        {"searchTerms", nlohmann::ordered_json::array()},
        {"foxglovePanelTitle", "System Logs"}
    };
}

// ════════════════════════════════════════════════════════════════════════════
// ROOT LAYOUT
// ════════════════════════════════════════════════════════════════════════════

void FoxgloveLayoutCreator::writeLayout(nlohmann::ordered_json& root)
{
    // Control area: mission column (Start/Pause/Abort) | fleet column (Arm/Land/Home)
    const nlohmann::ordered_json mission_col = {
        {"first", {
            {"first", "button.Button!mstart"},
            {"second", "button.Button!mpause"},
            {"direction", "column"},
            {"splitPercentage", 50}
        }},
        {"second", "button.Button!mabort"},
        {"direction", "column"},
        {"splitPercentage", 66.66}
    };
    const nlohmann::ordered_json fleet_col = {
        {"first", {
            {"first", "button.Button!armall"},
            {"second", "button.Button!landall"},
            {"direction", "column"},
            {"splitPercentage", 50}
        }},
        {"second", "button.Button!rth"},
        {"direction", "column"},
        {"splitPercentage", 66.66}
    };

    // Bottom area: logs (left 50%) | mission+fleet controls (right 50%)
    const nlohmann::ordered_json control_col = {
        {"first", mission_col},
        {"second", fleet_col},
        {"direction", "row"},
        {"splitPercentage", 50}
    };

    root["layout"] = {
        {"first", {
            {"first", "Tab!overview"},
            {"second", {
                {"first", "RosOut!log"},
                {"second", control_col},
                {"direction", "row"},
                {"splitPercentage", 50}
            }},
            {"direction", "column"},
            {"splitPercentage", 70}
        }},
        {"second", "Tab!cameras"},
        {"direction", "row"},
        {"splitPercentage", 50}
    };
}

// ════════════════════════════════════════════════════════════════════════════
// HELPERS
// ════════════════════════════════════════════════════════════════════════════

std::vector<std::string> FoxgloveLayoutCreator::getViewIds(const AgentConfigPtr& agent_ptr)
{
    std::vector<std::string> views;
    
    // Central camera ID first
    for (const auto& [camera_id, camera_ptr] : agent_ptr->tracking.multi_camera_set)
    {
        if (camera_ptr->role == ObservationRole::Central)
        {
            views.push_back(camera_id);
            break;
        }
    }

    // Then tracking camera IDs in map order
    for (const auto& [camera_id, camera_ptr] : agent_ptr->tracking.multi_camera_set)
    {
        if (camera_ptr->role != ObservationRole::Central)
        {
            views.push_back(camera_id);
        }
    }

    // Then crop window IDs in map order
    for (const auto& [window_id, window_ptr] : agent_ptr->tracking.multi_window_set)
    {
        views.push_back(window_id);
    }

    return views;
}