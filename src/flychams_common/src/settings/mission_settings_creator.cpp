#include "flychams_common/settings/mission_settings_creator.hpp"

using namespace flychams::common;

// ════════════════════════════════════════════════════════════════════════════
// YAML CREATION: Static methods for creating mission.yaml file
// ════════════════════════════════════════════════════════════════════════════

bool MissionSettingsCreator::createMissionSettings(const MissionConfigPtr& config_ptr, const std::string& path)
{
	std::ostringstream yaml_content;

	// Write YAML header comment
	yaml_content << "# Mission configuration file\n";
	yaml_content << "# Generated automatically from configuration spreadsheet\n";
	yaml_content << "# Mission ID: " << config_ptr->id << "\n";
	yaml_content << "# Mission Name: " << config_ptr->name << "\n\n";

	// Wrap under /**: and ros__parameters:
	yaml_content << "/**:\n";
	yaml_content << "  ros__parameters:\n";

	// Write mission section
	writeMissionSection(yaml_content, config_ptr);

	// Write environment section
	writeEnvironmentSection(yaml_content, config_ptr);

	// Write targets section
	writeTargetsSection(yaml_content, config_ptr);

	// Write agents section
	writeAgentsSection(yaml_content, config_ptr);

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
		std::cerr << "Error writing mission.yaml to file " << path << std::endl;
		return false;
	}
}

void MissionSettingsCreator::writeMissionSection(std::ostringstream& yaml, const MissionConfigPtr& config_ptr)
{
	yaml << "    mission:\n";
	yaml << "      id: " << config_ptr->id << "\n";
	yaml << "      name: \"" << config_ptr->name << "\"\n";
	yaml << "      environment_id: " << config_ptr->environment_id << "\n";
	yaml << "      target_group_id: " << config_ptr->target_group_id << "\n";
	yaml << "      agent_team_id: " << config_ptr->agent_team_id << "\n";
	yaml << "      horizontal_constraint:\n";
	yaml << "        min: " << std::fixed << std::setprecision(3) << config_ptr->horizontal_constraint(0) << "\n";
	yaml << "        max: " << std::fixed << std::setprecision(3) << config_ptr->horizontal_constraint(1) << "\n";
	yaml << "      vertical_constraint:\n";
	yaml << "        min: " << std::fixed << std::setprecision(3) << config_ptr->vertical_constraint(0) << "\n";
	yaml << "        max: " << std::fixed << std::setprecision(3) << config_ptr->vertical_constraint(1) << "\n";
	yaml << "      autopilot: " << static_cast<int>(config_ptr->autopilot) << "\n";
	yaml << "      start_date:\n";
	yaml << "        year: " << config_ptr->start_date.year << "\n";
	yaml << "        month: " << config_ptr->start_date.month << "\n";
	yaml << "        day: " << config_ptr->start_date.day << "\n";
	yaml << "      start_hour:\n";
	yaml << "        hours: " << config_ptr->start_hour.hours << "\n";
	yaml << "        minutes: " << config_ptr->start_hour.minutes << "\n";
	yaml << "        seconds: " << config_ptr->start_hour.seconds << "\n";
}

void MissionSettingsCreator::writeEnvironmentSection(std::ostringstream& yaml, const MissionConfigPtr& config_ptr)
{
	yaml << "    environment:\n";
	yaml << "      id: " << config_ptr->environment.id << "\n";
	yaml << "      name: \"" << config_ptr->environment.name << "\"\n";
	yaml << "      geopoint:\n";
	yaml << "        latitude: " << std::fixed << std::setprecision(6) << config_ptr->environment.geopoint.latitude << "\n";
	yaml << "        longitude: " << std::fixed << std::setprecision(6) << config_ptr->environment.geopoint.longitude << "\n";
	yaml << "        altitude: " << std::fixed << std::setprecision(6) << config_ptr->environment.geopoint.altitude << "\n";
	yaml << "      wind_vel:\n";
	yaml << "        x: " << std::fixed << std::setprecision(3) << config_ptr->environment.wind_vel.x() << "\n";
	yaml << "        y: " << std::fixed << std::setprecision(3) << config_ptr->environment.wind_vel.y() << "\n";
	yaml << "        z: " << std::fixed << std::setprecision(3) << config_ptr->environment.wind_vel.z() << "\n";
}

void MissionSettingsCreator::writeTargetsSection(std::ostringstream& yaml, const MissionConfigPtr& config_ptr)
{
	yaml << "    targets:\n";

	// Write id_list
	yaml << "      id_list: [";
	bool first = true;
	for (const auto& [target_id, target] : config_ptr->target_group)
	{
		if (!first)
		{
			yaml << ", ";
		}
		yaml << "\"" << target_id << "\"";
		first = false;
	}
	yaml << "]\n";

	// Write individual targets
	for (const auto& [target_id, target] : config_ptr->target_group)
	{
		yaml << "      " << target_id << ":\n";
		yaml << "        id: " << target_id << "\n";
		yaml << "        name: \"" << target->name << "\"\n";
		yaml << "        target_group_id: " << target->target_group_id << "\n";
		yaml << "        target_index: " << target->target_index << "\n";
		yaml << "        type: " << static_cast<int>(target->type) << "\n";
		yaml << "        count: " << target->count << "\n";
		yaml << "        priority: " << static_cast<int>(target->priority) << "\n";
		yaml << "        trajectory_folder: \"" << target->trajectory_folder << "\"\n";
	}
}

void MissionSettingsCreator::writeAgentsSection(std::ostringstream& yaml, const MissionConfigPtr& config_ptr)
{
	yaml << "    agents:\n";

	// Write id_list
	yaml << "      id_list: [";
	bool first = true;
	for (const auto& [agent_id, agent] : config_ptr->agent_team)
	{
		if (!first)
		{
			yaml << ", ";
		}
		yaml << "\"" << agent_id << "\"";
		first = false;
	}
	yaml << "]\n";

	// Write individual agents
	for (const auto& [agent_id, agent] : config_ptr->agent_team)
	{
		yaml << "      " << agent_id << ":\n";
		yaml << "        id: " << agent_id << "\n";
		yaml << "        idx: " << agent->idx << "\n";
		yaml << "        name: \"" << agent->name << "\"\n";
		yaml << "        agent_team_id: " << agent->agent_team_id << "\n";
		yaml << "        tracking_id: " << agent->tracking_id << "\n";
		yaml << "        drone_id: " << agent->drone_id << "\n";
		yaml << "        position:\n";
		yaml << "          x: " << std::fixed << std::setprecision(3) << agent->position.x() << "\n";
		yaml << "          y: " << std::fixed << std::setprecision(3) << agent->position.y() << "\n";
		yaml << "          z: " << std::fixed << std::setprecision(3) << agent->position.z() << "\n";
		yaml << "        orientation:\n";
		yaml << "          roll: " << std::fixed << std::setprecision(6) << agent->orientation.x() << "\n";
		yaml << "          pitch: " << std::fixed << std::setprecision(6) << agent->orientation.y() << "\n";
		yaml << "          yaw: " << std::fixed << std::setprecision(6) << agent->orientation.z() << "\n";
		yaml << "        safety_radius: " << std::fixed << std::setprecision(3) << agent->safety_radius << "\n";
		yaml << "        max_altitude: " << std::fixed << std::setprecision(3) << agent->max_altitude << "\n";
		yaml << "        battery_capacity: " << std::fixed << std::setprecision(3) << agent->battery_capacity << "\n";

		// Write tracking configuration
		yaml << "        tracking:\n";
		writeTrackingSection(yaml, agent->tracking, "          ");

		// Write drone configuration
		yaml << "        drone:\n";
		writeDroneSection(yaml, agent->drone, "          ");

		// Write SSH configuration
		yaml << "        ssh:\n";
		yaml << "          hostname: \"" << "172.17.0.1" << "\"\n";
		yaml << "          user: \"" << "jetson" << "\"\n";
	}
}

void MissionSettingsCreator::writeTrackingSection(std::ostringstream& yaml, const TrackingConfig& tracking, const std::string& prefix)
{
	yaml << prefix << "id: " << tracking.id << "\n";
	yaml << prefix << "name: \"" << tracking.name << "\"\n";
	yaml << prefix << "observation_set_id: " << tracking.observation_set_id << "\n";
	yaml << prefix << "min_target_size: " << std::fixed << std::setprecision(3) << tracking.min_target_size << "\n";
	yaml << prefix << "max_target_size: " << std::fixed << std::setprecision(3) << tracking.max_target_size << "\n";
	yaml << prefix << "ref_target_size: " << std::fixed << std::setprecision(3) << tracking.ref_target_size << "\n";

	if (!tracking.multi_camera_set.empty())
	{
		yaml << prefix << "multi_cameras:\n";

		// Write ids list
		yaml << prefix << "  ids: [";
		bool first = true;
		for (const auto& [multi_camera_id, multi_camera] : tracking.multi_camera_set)
		{
			if (!first)
			{
				yaml << ", ";
			}
			yaml << "\"" << multi_camera_id << "\"";
			first = false;
		}
		yaml << "]\n";

		// Write individual multi_cameras
		for (const auto& [multi_camera_id, multi_camera] : tracking.multi_camera_set)
		{
			yaml << prefix << "  " << multi_camera_id << ":\n";
			yaml << prefix << "    id: " << multi_camera_id << "\n";
			writeMultiCameraSection(yaml, multi_camera, prefix + "    ");
		}
	}

	if (!tracking.multi_window_set.empty())
	{
		yaml << prefix << "multi_windows:\n";

		// Write ids list
		yaml << prefix << "  ids: [";
		bool first = true;
		for (const auto& [multi_window_id, multi_window] : tracking.multi_window_set)
		{
			if (!first)
			{
				yaml << ", ";
			}
			yaml << "\"" << multi_window_id << "\"";
			first = false;
		}
		yaml << "]\n";

		// Write individual multi_windows
		for (const auto& [multi_window_id, multi_window] : tracking.multi_window_set)
		{
			yaml << prefix << "  " << multi_window_id << ":\n";
			yaml << prefix << "    id: " << multi_window_id << "\n";
			writeMultiWindowSection(yaml, multi_window, prefix + "    ");
		}
	}
}

void MissionSettingsCreator::writeMultiCameraSection(std::ostringstream& yaml, const MultiCameraConfigPtr& multi_camera, const std::string& prefix)
{
	yaml << prefix << "name: \"" << multi_camera->name << "\"\n";
	yaml << prefix << "observation_set_id: " << multi_camera->observation_set_id << "\n";
	yaml << prefix << "camera_id: " << multi_camera->camera_id << "\n";
	yaml << prefix << "gimbal_id: " << multi_camera->gimbal_id << "\n";
	yaml << prefix << "role: " << static_cast<int>(multi_camera->role) << "\n";
	yaml << prefix << "position:\n";
	yaml << prefix << "  x: " << std::fixed << std::setprecision(3) << multi_camera->position.x() << "\n";
	yaml << prefix << "  y: " << std::fixed << std::setprecision(3) << multi_camera->position.y() << "\n";
	yaml << prefix << "  z: " << std::fixed << std::setprecision(3) << multi_camera->position.z() << "\n";
	yaml << prefix << "orientation:\n";
	yaml << prefix << "  roll: " << std::fixed << std::setprecision(6) << multi_camera->orientation.x() << "\n";
	yaml << prefix << "  pitch: " << std::fixed << std::setprecision(6) << multi_camera->orientation.y() << "\n";
	yaml << prefix << "  yaw: " << std::fixed << std::setprecision(6) << multi_camera->orientation.z() << "\n";
	yaml << prefix << "min_focal: " << std::fixed << std::setprecision(6) << multi_camera->min_focal << "\n";
	yaml << prefix << "max_focal: " << std::fixed << std::setprecision(6) << multi_camera->max_focal << "\n";
	yaml << prefix << "ref_focal: " << std::fixed << std::setprecision(6) << multi_camera->ref_focal << "\n";
	yaml << prefix << "source_stream_url: \"" << multi_camera->source_stream_url << "\"\n";

	yaml << prefix << "camera:\n";
	writeCameraSection(yaml, multi_camera->camera, prefix + "  ");

	yaml << prefix << "gimbal:\n";
	writeGimbalSection(yaml, multi_camera->gimbal, prefix + "  ");
}

void MissionSettingsCreator::writeMultiWindowSection(std::ostringstream& yaml, const MultiWindowConfigPtr& multi_window, const std::string& prefix)
{
	yaml << prefix << "name: \"" << multi_window->name << "\"\n";
	yaml << prefix << "observation_set_id: " << multi_window->observation_set_id << "\n";
	yaml << prefix << "resolution:\n";
	yaml << prefix << "  width: " << std::fixed << std::setprecision(3) << multi_window->resolution(0) << "\n";
	yaml << prefix << "  height: " << std::fixed << std::setprecision(3) << multi_window->resolution(1) << "\n";
	yaml << prefix << "min_lambda: " << std::fixed << std::setprecision(3) << multi_window->min_lambda << "\n";
	yaml << prefix << "max_lambda: " << std::fixed << std::setprecision(3) << multi_window->max_lambda << "\n";
	yaml << prefix << "ref_lambda: " << std::fixed << std::setprecision(3) << multi_window->ref_lambda << "\n";
}

void MissionSettingsCreator::writeCameraSection(std::ostringstream& yaml, const CameraConfig& camera, const std::string& prefix)
{
	yaml << prefix << "id: " << camera.id << "\n";
	yaml << prefix << "name: \"" << camera.name << "\"\n";
	yaml << prefix << "type: " << static_cast<int>(camera.type) << "\n";
	yaml << prefix << "resolution:\n";
	yaml << prefix << "  width: " << std::fixed << std::setprecision(3) << camera.resolution(0) << "\n";
	yaml << prefix << "  height: " << std::fixed << std::setprecision(3) << camera.resolution(1) << "\n";
	yaml << prefix << "sensor_size:\n";
	yaml << prefix << "  width: " << std::fixed << std::setprecision(6) << camera.sensor_size(0) << "\n";
	yaml << prefix << "  height: " << std::fixed << std::setprecision(6) << camera.sensor_size(1) << "\n";
	yaml << prefix << "distortion:\n";
	yaml << prefix << "  K1: " << std::fixed << std::setprecision(6) << camera.distortion.K1 << "\n";
	yaml << prefix << "  K2: " << std::fixed << std::setprecision(6) << camera.distortion.K2 << "\n";
	yaml << prefix << "  K3: " << std::fixed << std::setprecision(6) << camera.distortion.K3 << "\n";
	yaml << prefix << "  P1: " << std::fixed << std::setprecision(6) << camera.distortion.P1 << "\n";
	yaml << prefix << "  P2: " << std::fixed << std::setprecision(6) << camera.distortion.P2 << "\n";
	yaml << prefix << "enable_sensor_noise: " << (camera.enable_sensor_noise ? "true" : "false") << "\n";
	if (camera.enable_sensor_noise)
	{
		yaml << prefix << "sensor_noise:\n";
		yaml << prefix << "  rand_contrib: " << std::fixed << std::setprecision(6) << camera.sensor_noise.rand_contrib << "\n";
		yaml << prefix << "  rand_size: " << std::fixed << std::setprecision(6) << camera.sensor_noise.rand_size << "\n";
		yaml << prefix << "  rand_speed: " << std::fixed << std::setprecision(6) << camera.sensor_noise.rand_speed << "\n";
	}
	yaml << prefix << "weight: " << std::fixed << std::setprecision(3) << camera.weight << "\n";
	yaml << prefix << "idle_power: " << std::fixed << std::setprecision(3) << camera.idle_power << "\n";
	yaml << prefix << "active_power: " << std::fixed << std::setprecision(3) << camera.active_power << "\n";
}

void MissionSettingsCreator::writeGimbalSection(std::ostringstream& yaml, const GimbalConfig& gimbal, const std::string& prefix)
{
	yaml << prefix << "id: " << gimbal.id << "\n";
	yaml << prefix << "name: \"" << gimbal.name << "\"\n";
	yaml << prefix << "enable_roll: " << (gimbal.enable_roll ? "true" : "false") << "\n";
	if (gimbal.enable_roll)
	{
		yaml << prefix << "roll:\n";
		yaml << prefix << "  min_angle: " << std::fixed << std::setprecision(6) << gimbal.roll.min_angle << "\n";
		yaml << prefix << "  max_angle: " << std::fixed << std::setprecision(6) << gimbal.roll.max_angle << "\n";
		yaml << prefix << "  max_speed: " << std::fixed << std::setprecision(6) << gimbal.roll.max_speed << "\n";
	}
	yaml << prefix << "enable_pitch: " << (gimbal.enable_pitch ? "true" : "false") << "\n";
	if (gimbal.enable_pitch)
	{
		yaml << prefix << "pitch:\n";
		yaml << prefix << "  min_angle: " << std::fixed << std::setprecision(6) << gimbal.pitch.min_angle << "\n";
		yaml << prefix << "  max_angle: " << std::fixed << std::setprecision(6) << gimbal.pitch.max_angle << "\n";
		yaml << prefix << "  max_speed: " << std::fixed << std::setprecision(6) << gimbal.pitch.max_speed << "\n";
	}
	yaml << prefix << "enable_yaw: " << (gimbal.enable_yaw ? "true" : "false") << "\n";
	if (gimbal.enable_yaw)
	{
		yaml << prefix << "yaw:\n";
		yaml << prefix << "  min_angle: " << std::fixed << std::setprecision(6) << gimbal.yaw.min_angle << "\n";
		yaml << prefix << "  max_angle: " << std::fixed << std::setprecision(6) << gimbal.yaw.max_angle << "\n";
		yaml << prefix << "  max_speed: " << std::fixed << std::setprecision(6) << gimbal.yaw.max_speed << "\n";
	}
	yaml << prefix << "weight: " << std::fixed << std::setprecision(3) << gimbal.weight << "\n";
	yaml << prefix << "idle_power: " << std::fixed << std::setprecision(3) << gimbal.idle_power << "\n";
	yaml << prefix << "active_power: " << std::fixed << std::setprecision(3) << gimbal.active_power << "\n";
}

void MissionSettingsCreator::writeDroneSection(std::ostringstream& yaml, const DroneConfig& drone, const std::string& prefix)
{
	yaml << prefix << "id: " << drone.id << "\n";
	yaml << prefix << "name: \"" << drone.name << "\"\n";
	yaml << prefix << "type: " << static_cast<int>(drone.type) << "\n";
	yaml << prefix << "cruise_speed: " << std::fixed << std::setprecision(3) << drone.cruise_speed << "\n";
	yaml << prefix << "max_speed: " << std::fixed << std::setprecision(3) << drone.max_speed << "\n";

	yaml << prefix << "enable_barometer: " << (drone.enable_barometer ? "true" : "false") << "\n";
	if (drone.enable_barometer)
	{
		yaml << prefix << "barometer:\n";
		yaml << prefix << "  white_noise_sigma: " << std::fixed << std::setprecision(6) << drone.barometer.white_noise_sigma << "\n";
	}

	yaml << prefix << "enable_imu: " << (drone.enable_imu ? "true" : "false") << "\n";
	if (drone.enable_imu)
	{
		yaml << prefix << "imu:\n";
		yaml << prefix << "  angular_white_noise_sigma: " << std::fixed << std::setprecision(6) << drone.imu.angular_white_noise_sigma << "\n";
		yaml << prefix << "  velocity_white_noise_sigma: " << std::fixed << std::setprecision(6) << drone.imu.velocity_white_noise_sigma << "\n";
	}

	yaml << prefix << "enable_gps: " << (drone.enable_gps ? "true" : "false") << "\n";
	if (drone.enable_gps)
	{
		yaml << prefix << "gps:\n";
		yaml << prefix << "  eph_initial: " << std::fixed << std::setprecision(6) << drone.gps.eph_initial << "\n";
		yaml << prefix << "  epv_initial: " << std::fixed << std::setprecision(6) << drone.gps.epv_initial << "\n";
		yaml << prefix << "  eph_final: " << std::fixed << std::setprecision(6) << drone.gps.eph_final << "\n";
		yaml << prefix << "  epv_final: " << std::fixed << std::setprecision(6) << drone.gps.epv_final << "\n";
	}

	yaml << prefix << "enable_magnetometer: " << (drone.enable_magnetometer ? "true" : "false") << "\n";
	if (drone.enable_magnetometer)
	{
		yaml << prefix << "magnetometer:\n";
		yaml << prefix << "  white_noise_sigma: " << std::fixed << std::setprecision(6) << drone.magnetometer.white_noise_sigma << "\n";
		yaml << prefix << "  white_noise_bias: " << std::fixed << std::setprecision(6) << drone.magnetometer.white_noise_bias << "\n";
	}

	yaml << prefix << "base_weight: " << std::fixed << std::setprecision(3) << drone.base_weight << "\n";
	yaml << prefix << "max_payload_weight: " << std::fixed << std::setprecision(3) << drone.max_payload_weight << "\n";
	yaml << prefix << "hover_power: " << std::fixed << std::setprecision(3) << drone.hover_power << "\n";
	yaml << prefix << "cruise_power: " << std::fixed << std::setprecision(3) << drone.cruise_power << "\n";
	yaml << prefix << "load_factor: " << std::fixed << std::setprecision(3) << drone.load_factor << "\n";
}