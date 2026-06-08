#include "flychams_common/settings/airsim_settings_creator.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// JSON CREATION: Static methods for creating settings.json file
// ════════════════════════════════════════════════════════════════════════════

bool AirsimSettingsCreator::createAirsimSettings(const MissionConfigPtr& config_ptr, const std::string& path)
{
    // Generate settings.json content
    nlohmann::ordered_json settings;
    writeGeneralSection(config_ptr, settings);
    writeVehiclesSection(config_ptr, settings["Vehicles"]);
    writeSubWindowsSection(config_ptr, settings["SubWindows"]);
    writeStreamsSection(config_ptr, settings["Streams"]);
    writeQualitySettingsSection(config_ptr, settings["QualitySettings"]);

    // Write settings to file
    std::ofstream file(path);
    if (file.is_open())
    {
        file << settings.dump(4);
        file.close();
    }
    else
    {
        std::cerr << "Error writing settings to file " << path << std::endl;
        return false;
    }

    return true;
}

void AirsimSettingsCreator::writeGeneralSection(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& settings)
{
    // General settings
    settings["SettingsVersion"] = 2.0;
    settings["SimMode"] = "Multirotor";
    settings["ClockType"] = "SteppableClock";
    settings["ClockSpeed"] = config_ptr->system.clock_speed;
    settings["ViewMode"] = "NoDisplay";
    settings["LogMessagesVisible"] = false;
    settings["ApiServerPort"] = 41451;

    // Pawn paths
    settings["PawnPaths"] = {
        {"FlyChamsQuadcopter", {{"PawnBP", "Class'/AirSim/Blueprints/BP_FlyChamsQuadcopter.BP_FlyChamsQuadcopter_C'"}}},
        {"FlyChamsHexacopter", {{"PawnBP", "Class'/AirSim/Blueprints/BP_FlyChamsHexacopter.BP_FlyChamsHexacopter_C'"}}} };

    // Origin geopoint
    settings["OriginGeopoint"] = {
        {"Latitude", config_ptr->environment.geopoint.latitude},
        {"Longitude", config_ptr->environment.geopoint.longitude},
        {"Altitude", config_ptr->environment.geopoint.altitude} };

    // Time of day (format: %Y-%m-%d %H:%M:%S)
    const auto& date = config_ptr->start_date;
    const auto& hour = config_ptr->start_hour;
    settings["TimeOfDay"] = {
        {"Enabled", true},
        {"StartDateTime",
            std::to_string(date.year) + "-" +
                std::to_string(date.month) + "-" +
                std::to_string(date.day) + " " +
                std::to_string(hour.hours) + ":" +
                std::to_string(hour.minutes) + ":" +
                std::to_string(hour.seconds)},
        {"CelestialClockSpeed", 1},
        {"StartDateTimeDst", false},
        {"UpdateIntervalSecs", 1} };

    // Wind velocity
    const auto& wind_vel = config_ptr->environment.wind_vel;
    settings["Wind"] = {
        {"X", wind_vel.x()},
        {"Y", -wind_vel.y()},
        {"Z", -wind_vel.z()} };

    // Camera defaults
    settings["CameraDefaults"] = {
        {"CaptureSettings", {
            {
                {"ImageType", 0},
                {"Width", 1920},
                {"Height", 1080},
                {"SensorWidth", 0.0132f},
                {"SensorHeight", 0.007425f},
                {"FOV_Degrees", 90},
                {"LumenGIEnable", false},
                {"LumenReflectionEnable", false},
                {"LumenFinalQuality", 0},
                {"LumenSceneDetail", 0},
                {"LumenSceneLightningDetail", 0}
            }
        }}
    };
}

void AirsimSettingsCreator::writeQualitySettingsSection(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& quality_settings)
{
    int quality_preset = config_ptr->system.quality_preset;
    if (quality_preset < 0)
        quality_preset = 0;
    else if (quality_preset > 3)
        quality_preset = 3;

    switch (quality_preset)
    {
    case 0:
        quality_settings = {
            {"FrameRate", {
                {"MaxFPS", 20},
                {"CameraCaptureFPS", 10}
            }},
            {"Scalability", {
                {"ResolutionQuality", 50},
                {"ViewDistanceQuality", 0},
                {"AntiAliasingQuality", 0},
                {"ShadowQuality", 0},
                {"GlobalIlluminationQuality", 0},
                {"ReflectionQuality", 0},
                {"PostProcessQuality", 0},
                {"TextureQuality", 0},
                {"EffectsQuality", 0},
                {"FoliageQuality", 0},
                {"ShadingQuality", 0}
            }},
            {"RenderingFeatures", {
                {"MotionBlur", false},
                {"Bloom", false},
                {"AmbientOcclusion", false},
                {"ScreenSpaceReflections", false},
                {"VolumetricFog", false},
                {"LensFlares", false},
                {"DepthOfField", false},
                {"ContactShadows", false}
            }},
            {"World", {
                {"FoliageDensityScale", 0.1},
                {"GrassDensityScale", 0.0},
                {"ViewDistanceScale", 0.35},
                {"ShadowDistanceScale", 0.1}
            }}
        };
        break;
    case 2:
        quality_settings = {
            {"FrameRate", {
                {"MaxFPS", 45},
                {"CameraCaptureFPS", 20}
            }},
            {"Scalability", {
                {"ResolutionQuality", 85},
                {"ViewDistanceQuality", 2},
                {"AntiAliasingQuality", 2},
                {"ShadowQuality", 1},
                {"GlobalIlluminationQuality", 0},
                {"ReflectionQuality", 1},
                {"PostProcessQuality", 1},
                {"TextureQuality", 2},
                {"EffectsQuality", 2},
                {"FoliageQuality", 1},
                {"ShadingQuality", 2}
            }},
            {"RenderingFeatures", {
                {"MotionBlur", false},
                {"Bloom", false},
                {"AmbientOcclusion", false},
                {"ScreenSpaceReflections", false},
                {"VolumetricFog", false},
                {"LensFlares", false},
                {"DepthOfField", false},
                {"ContactShadows", false}
            }},
            {"World", {
                {"FoliageDensityScale", 0.5},
                {"GrassDensityScale", 0.25},
                {"ViewDistanceScale", 0.75},
                {"ShadowDistanceScale", 0.5}
            }}
        };
        break;
    case 3:
        quality_settings = {
            {"FrameRate", {
                {"MaxFPS", 60},
                {"CameraCaptureFPS", 30}
            }},
            {"Scalability", {
                {"ResolutionQuality", 100},
                {"ViewDistanceQuality", 3},
                {"AntiAliasingQuality", 3},
                {"ShadowQuality", 2},
                {"GlobalIlluminationQuality", 1},
                {"ReflectionQuality", 2},
                {"PostProcessQuality", 2},
                {"TextureQuality", 3},
                {"EffectsQuality", 3},
                {"FoliageQuality", 2},
                {"ShadingQuality", 3}
            }},
            {"RenderingFeatures", {
                {"MotionBlur", false},
                {"Bloom", true},
                {"AmbientOcclusion", true},
                {"ScreenSpaceReflections", true},
                {"VolumetricFog", false},
                {"LensFlares", true},
                {"DepthOfField", false},
                {"ContactShadows", true}
            }},
            {"World", {
                {"FoliageDensityScale", 1.0},
                {"GrassDensityScale", 1.0},
                {"ViewDistanceScale", 1.0},
                {"ShadowDistanceScale", 1.0}
            }}
        };
        break;
    case 1:
    default:
        quality_settings = {
            {"FrameRate", {
                {"MaxFPS", 30},
                {"CameraCaptureFPS", 15}
            }},
            {"Scalability", {
                {"ResolutionQuality", 70},
                {"ViewDistanceQuality", 1},
                {"AntiAliasingQuality", 1},
                {"ShadowQuality", 0},
                {"GlobalIlluminationQuality", 0},
                {"ReflectionQuality", 0},
                {"PostProcessQuality", 0},
                {"TextureQuality", 1},
                {"EffectsQuality", 1},
                {"FoliageQuality", 0},
                {"ShadingQuality", 1}
            }},
            {"RenderingFeatures", {
                {"MotionBlur", false},
                {"Bloom", false},
                {"AmbientOcclusion", false},
                {"ScreenSpaceReflections", false},
                {"VolumetricFog", false},
                {"LensFlares", false},
                {"DepthOfField", false},
                {"ContactShadows", false}
            }},
            {"World", {
                {"FoliageDensityScale", 0.25},
                {"GrassDensityScale", 0.0},
                {"ViewDistanceScale", 0.5},
                {"ShadowDistanceScale", 0.25}
            }}
        };
        break;
    }
}

void AirsimSettingsCreator::writeVehiclesSection(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& vehicles)
{
    int instance = 0;
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        // Get relevant config
        const auto& drone = agent_ptr->drone;
        Vector3r ini_pos = agent_ptr->position;
        Vector3r ini_ori = agent_ptr->orientation;

        vehicles[agent_id] = {
            {"PawnPath", drone.type == DroneType::Quadcopter ? "FlyChamsQuadcopter" : "FlyChamsHexacopter"},
            {"X", ini_pos.x()},
            {"Y", -ini_pos.y()},
            {"Z", -ini_pos.z()},
            {"Roll", MathUtils::radToDeg(ini_ori.x())},
            {"Pitch", MathUtils::radToDeg(-ini_ori.y())},
            {"Yaw", MathUtils::radToDeg(-ini_ori.z())}
        };

        if (config_ptr->autopilot == Autopilot::PX4)
        {
            vehicles[agent_id]["VehicleType"] = "PX4Multirotor";
            vehicles[agent_id]["Model"] = drone.type == DroneType::Quadcopter ? "FlyChamsQuadcopter" : "FlyChamsHexacopter";
            vehicles[agent_id]["UseSerial"] = false;
            vehicles[agent_id]["LockStep"] = true;
            vehicles[agent_id]["UseTcp"] = true;
            vehicles[agent_id]["TcpPort"] = 4560 + instance;
            vehicles[agent_id]["ControlIp"] = "remote";
            vehicles[agent_id]["ControlPortLocal"] = 14540 + instance;
            vehicles[agent_id]["ControlPortRemote"] = 14580 + instance;
            vehicles[agent_id]["LocalHostIp"] = "172.17.0.1";
            vehicles[agent_id]["Parameters"] = {
                {"MPC_XY_VEL_MAX", 2.5},
                {"NAV_RCL_ACT", 0},
                {"NAV_DLL_ACT", 0},
                {"COM_OBL_RC_ACT", 1},
                {"COM_DISARM_PRFLT", 0},
                {"COM_RC_IN_MODE", 1},
                {"LPE_LAT", config_ptr->environment.geopoint.latitude},
                {"LPE_LON", config_ptr->environment.geopoint.longitude},
                {"SENS_BARO_RATE", 20.0 * config_ptr->system.clock_speed}
            };
        }
        else
        {
            vehicles[agent_id]["VehicleType"] = "SimpleFlight";
            vehicles[agent_id]["DefaultVehicleState"] = "Armed";
            vehicles[agent_id]["AutoCreate"] = true;
        }

        // Add sensors to the vehicle
        writeSensorsSection(agent_id, config_ptr, vehicles[agent_id]["Sensors"]);

        // Add cameras to the vehicle
        writeInternalCamerasSection(agent_id, config_ptr, vehicles[agent_id]["Cameras"]);

        // Add external cameras to the vehicle
        writeExternalCamerasSection(agent_id, instance == 0, config_ptr, vehicles[agent_id]["Cameras"]);

        instance++;
    }
}

// Helper method: Populate sensors
void AirsimSettingsCreator::writeSensorsSection(const ID& agent_id, const MissionConfigPtr& config_ptr, nlohmann::ordered_json& sensors)
{
    // Get relevant config
    const auto& drone = config_ptr->agent_team[agent_id]->drone;

    sensors = {
        {"Barometer", {
            {"SensorType", 1},
                {"Enabled", drone.enable_barometer},
                {"PressureFactorSigma", 0.0001825f}, // More than 0.0001825 can generate problems with PX4
                {"UncorrelatedNoiseSigma", drone.barometer.white_noise_sigma}
            }},
            {"Imu", {
                {"SensorType", 2},
                {"Enabled", drone.enable_imu},
                {"GenerateNoise", true},
                {"AngularRandomWalk", drone.imu.angular_white_noise_sigma},
                {"VelocityRandomWalk", drone.imu.velocity_white_noise_sigma}
            }},
            {"Gps", {
                {"SensorType", 3},
                {"Enabled", drone.enable_gps},
                {"EphInitial", drone.gps.eph_initial},
                {"EpvInitial", drone.gps.epv_initial},
                {"EphFinal", drone.gps.eph_final},
                {"EpvFinal", drone.gps.epv_final}
            }},
            {"Magnetometer", {
                {"SensorType", 4},
                {"Enabled", drone.enable_magnetometer},
                {"NoiseSigma", drone.magnetometer.white_noise_sigma},
                {"NoiseBias", drone.magnetometer.white_noise_bias}
        }}
    };
}

// Helper method: Populate cameras
void AirsimSettingsCreator::writeInternalCamerasSection(const ID& agent_id, const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cameras)
{
    for (const auto& [multi_camera_id, multi_camera_ptr] : config_ptr->agent_team[agent_id]->tracking.multi_camera_set)
    {
        // Get relevant config
        const auto& gimbal = multi_camera_ptr->gimbal;
        const auto& camera = multi_camera_ptr->camera;
        const auto& distortion = camera.distortion;
        const auto& mount_pos = multi_camera_ptr->position;
        const auto& mount_ori = multi_camera_ptr->orientation;

        cameras[multi_camera_id] = {
            {"CaptureSettings", {
                {
                    {"ImageType", 0},
                    {"Width", camera.resolution(0)},
                    {"Height", camera.resolution(1)},
                    {"SensorWidth", camera.sensor_size(0)},
                    {"SensorHeight", camera.sensor_size(1)},
                    {"FOV_Degrees", MathUtils::radToDeg(VisionUtils::computeFov(multi_camera_ptr->ref_focal, camera.sensor_size(0)))},
                    {"K1", distortion.K1},
                    {"K2", distortion.K2},
                    {"K3", distortion.K3},
                    {"P1", distortion.P1},
                    {"P2", distortion.P2},
                    {"LumenGIEnable", false},
                    {"LumenReflectionEnable", false},
                    {"LumenFinalQuality", 0},
                    {"LumenSceneDetail", 0},
                    {"LumenSceneLightningDetail", 0}
                }
            }},
            {"Gimbal", {
                {"YawMin", gimbal.yaw.min_angle}, {"PitchMin", gimbal.pitch.min_angle}, {"RollMin", gimbal.roll.min_angle},
                {"YawMax", gimbal.yaw.max_angle}, {"PitchMax", gimbal.pitch.max_angle}, {"RollMax", gimbal.roll.max_angle},
                {"YawSpeed", gimbal.yaw.max_speed}, {"PitchSpeed", gimbal.pitch.max_speed}, {"RollSpeed", gimbal.roll.max_speed},
                {"Roll", MathUtils::radToDeg(mount_ori.x())}, {"Pitch", 0.0f}, {"Yaw", MathUtils::radToDeg(-mount_ori.z())}
            }},
            {"NoiseSettings", {
                {
                    {"Enabled", camera.enable_sensor_noise},
                    {"ImageType", 0},
                    {"RandContrib", camera.sensor_noise.rand_contrib},
                    {"RandSpeed", camera.sensor_noise.rand_speed},
                    {"RandSize", camera.sensor_noise.rand_size},
                    {"RandDensity", 2},
                    {"HorzWaveContrib", 0.0004f},
                    {"HorzWaveStrength", 0.0007f},
                    {"HorzWaveVertSize", 1.0f},
                    {"HorzWaveScreenSize", 1.0f},
                    {"HorzNoiseLinesContrib", 0.0008f},
                    {"HorzNoiseLinesDensityY", 0.0001f},
                    {"HorzNoiseLinesDensityXY", 0.004f},
                    {"HorzDistortionContrib", 0.0f},
                    {"HorzDistortionStrength", 0.0f}
                }
            }},
            {"X", mount_pos.x()}, {"Y", -mount_pos.y()}, {"Z", -mount_pos.z()},
            {"Roll", 0.0f}, {"Pitch", MathUtils::radToDeg(-mount_ori.y())}, {"Yaw", 0.0f},
            {"EnableGimbal", true}, {"CameraVisible", true}, {"CameraScale", 0.5f}
        };
    }
}

void AirsimSettingsCreator::writeExternalCamerasSection(const ID& agent_id, bool is_first_agent, const MissionConfigPtr& config_ptr, nlohmann::ordered_json& cameras)
{
    // Scenario view camera (external, world-fixed — only registered once on the first agent)
    if (is_first_agent)
    {
        const auto& scenario_view_pos = config_ptr->system.scenario_camera_position;
        const auto& scenario_view_ori = config_ptr->system.scenario_camera_orientation;
        const auto& scenario_res = config_ptr->system.scenario_camera_resolution;

        cameras["SCENARIOCAM"] = {
            {"CaptureSettings", {{
                {"ImageType", 0},
                {"Width", scenario_res(0)},
                {"Height", scenario_res(1)},
                {"SensorWidth", 0.0132f},
                {"SensorHeight", 0.007425f},
                {"FOV_Degrees", 90},
                {"LumenGIEnable", false},
                {"LumenReflectionEnable", false},
                {"LumenFinalQuality", 0},
                {"LumenSceneDetail", 0},
                {"LumenSceneLightningDetail", 0}
            }}},
            {"X", scenario_view_pos.x()},
            {"Y", -scenario_view_pos.y()},
            {"Z", -scenario_view_pos.z()},
            {"Roll", MathUtils::radToDeg(scenario_view_ori.x())},
            {"Pitch", MathUtils::radToDeg(-scenario_view_ori.y())},
            {"Yaw", MathUtils::radToDeg(-scenario_view_ori.z())},
            {"External", true} };
    }

    // Agent view camera (agent-relative, one per agent)
    const auto& agent_view_pos = config_ptr->system.agent_camera_position;
    const auto& agent_view_ori = config_ptr->system.agent_camera_orientation;
    const auto& agent_res = config_ptr->system.agent_camera_resolution;

    cameras["AGENTCAM_" + agent_id] = {
        {"CaptureSettings", {{
            {"ImageType", 0},
            {"Width", agent_res(0)},
            {"Height", agent_res(1)},
            {"SensorWidth", 0.0132f},
            {"SensorHeight", 0.007425f},
            {"FOV_Degrees", 90},
            {"LumenGIEnable", false},
            {"LumenReflectionEnable", false},
            {"LumenFinalQuality", 0},
            {"LumenSceneDetail", 0},
            {"LumenSceneLightningDetail", 0}
        }}},
        {"X", agent_view_pos.x()},
        {"Y", -agent_view_pos.y()},
        {"Z", -agent_view_pos.z()},
        {"Roll", MathUtils::radToDeg(agent_view_ori.x())},
        {"Pitch", MathUtils::radToDeg(-agent_view_ori.y())},
        {"Yaw", MathUtils::radToDeg(-agent_view_ori.z())} };

    // Payload view camera (agent-relative, one per agent)
    const auto& payload_view_pos = config_ptr->system.payload_camera_position;
    const auto& payload_view_ori = config_ptr->system.payload_camera_orientation;
    const auto& payload_res = config_ptr->system.payload_camera_resolution;

    cameras["PAYLOADCAM_" + agent_id] = {
        {"CaptureSettings", {{
            {"ImageType", 0},
            {"Width", payload_res(0)},
            {"Height", payload_res(1)},
            {"SensorWidth", 0.0132f},
            {"SensorHeight", 0.007425f},
            {"FOV_Degrees", 90},
            {"LumenGIEnable", false},
            {"LumenReflectionEnable", false},
            {"LumenFinalQuality", 0},
            {"LumenSceneDetail", 0},
            {"LumenSceneLightningDetail", 0}
        }}},
        {"X", payload_view_pos.x()},
        {"Y", -payload_view_pos.y()},
        {"Z", -payload_view_pos.z()},
        {"Roll", MathUtils::radToDeg(payload_view_ori.x())},
        {"Pitch", MathUtils::radToDeg(-payload_view_ori.y())},
        {"Yaw", MathUtils::radToDeg(-payload_view_ori.z())} };
}

void AirsimSettingsCreator::writeSubWindowsSection(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& subwindows)
{
    int idx = 0;
    subwindows = nlohmann::ordered_json::array();

    // Get first agent id
    const auto& agent_id = config_ptr->agent_team.begin()->first;

    // Scene view sub-window
    subwindows.push_back({ {"WindowID", idx++},
                            {"CameraName", "SCENARIOCAM"},
                            {"ImageType", 0},
                            {"VehicleName", agent_id},
                            {"Visible", true} });

    // Agent view sub-window
    subwindows.push_back({ {"WindowID", idx++},
                            {"CameraName", "AGENTCAM"},
                            {"ImageType", 0},
                            {"VehicleName", agent_id},
                            {"Visible", true} });

    // Map view sub-window
    subwindows.push_back({ {"WindowID", idx++},
                            {"CameraName", "MAPVIEW"},
                            {"ImageType", 0},
                            {"VehicleName", ""},
                            {"Visible", false} });

    // Payload view sub-window
    subwindows.push_back({ {"WindowID", idx++},
                            {"CameraName", "PAYLOADCAM"},
                            {"ImageType", 0},
                            {"VehicleName", agent_id},
                            {"Visible", true} });

    // Tracking views sub-windows (use first agent's tracking views)
    for (const auto& [camera_id, camera_ptr] : config_ptr->agent_team[agent_id]->tracking.multi_camera_set)
    {
        subwindows.push_back({ {"WindowID", idx++},
                                {"CameraName", camera_id},
                                {"ImageType", 0},
                                {"VehicleName", agent_id},
                                {"Visible", false} });
    }
}

void AirsimSettingsCreator::writeStreamsSection(const MissionConfigPtr& config_ptr, nlohmann::ordered_json& streams)
{
    streams = nlohmann::ordered_json::array();

    int rtsp_port = 8554;
    const auto& first_agent_id = config_ptr->agent_team.begin()->first;

    // Scenario view stream (external camera, single instance on first agent)
    streams.push_back({ {"CameraName", "SCENARIOCAM"},
                        {"ImageType", 0},
                        {"VehicleName", first_agent_id},
                        {"RtspPort", rtsp_port} });

    // Agent and payload view streams (per-agent)
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        streams.push_back({ {"CameraName", "AGENTCAM_" + agent_id},
                            {"ImageType", 0},
                            {"VehicleName", agent_id},
                            {"RtspPort", rtsp_port} });

        streams.push_back({ {"CameraName", "PAYLOADCAM_" + agent_id},
                            {"ImageType", 0},
                            {"VehicleName", agent_id},
                            {"RtspPort", rtsp_port} });
    }

    // Tracking camera streams (per-agent)
    for (const auto& [agent_id, agent_ptr] : config_ptr->agent_team)
    {
        for (const auto& [camera_id, camera_ptr] : agent_ptr->tracking.multi_camera_set)
        {
            streams.push_back({ {"CameraName", camera_id},
                                {"ImageType", 0},
                                {"VehicleName", agent_id},
                                {"RtspPort", rtsp_port} });
        }
    }
}