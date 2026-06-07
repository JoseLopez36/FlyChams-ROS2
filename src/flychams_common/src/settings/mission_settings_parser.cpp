#include "flychams_common/settings/mission_settings_parser.hpp"

using namespace flychams::common;

void MissionSettingsParser::parseMissionParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
{
	node->get_parameter<std::string>(prefix + "id", config_ptr->id);
	node->get_parameter<std::string>(prefix + "name", config_ptr->name);

	node->get_parameter<std::string>(prefix + "environment_id", config_ptr->environment_id);
	node->get_parameter<std::string>(prefix + "target_group_id", config_ptr->target_group_id);
	node->get_parameter<std::string>(prefix + "agent_team_id", config_ptr->agent_team_id);

	node->get_parameter<float>(prefix + "horizontal_constraint.min", config_ptr->horizontal_constraint(0));
	node->get_parameter<float>(prefix + "horizontal_constraint.max", config_ptr->horizontal_constraint(1));

	node->get_parameter<float>(prefix + "vertical_constraint.min", config_ptr->vertical_constraint(0));
	node->get_parameter<float>(prefix + "vertical_constraint.max", config_ptr->vertical_constraint(1));

	int8_t autopilot_int;
	node->get_parameter<int8_t>(prefix + "autopilot", autopilot_int);
	config_ptr->autopilot = static_cast<Autopilot>(autopilot_int);

	int64_t year, month, day;
	node->get_parameter<int64_t>(prefix + "start_date.year", year);
	node->get_parameter<int64_t>(prefix + "start_date.month", month);
	node->get_parameter<int64_t>(prefix + "start_date.day", day);
	config_ptr->start_date.year = static_cast<uint32_t>(year);
	config_ptr->start_date.month = static_cast<uint32_t>(month);
	config_ptr->start_date.day = static_cast<uint32_t>(day);

	int64_t hours, minutes, seconds;
	node->get_parameter<int64_t>(prefix + "start_hour.hours", hours);
	node->get_parameter<int64_t>(prefix + "start_hour.minutes", minutes);
	node->get_parameter<int64_t>(prefix + "start_hour.seconds", seconds);
	config_ptr->start_hour.hours = static_cast<uint32_t>(hours);
	config_ptr->start_hour.minutes = static_cast<uint32_t>(minutes);
	config_ptr->start_hour.seconds = static_cast<uint32_t>(seconds);
}

void MissionSettingsParser::parseEnvironmentParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
{
	node->get_parameter<std::string>(prefix + "id", config_ptr->environment.id);
	node->get_parameter<std::string>(prefix + "name", config_ptr->environment.name);

	node->get_parameter<double>(prefix + "geopoint.latitude", config_ptr->environment.geopoint.latitude);
	node->get_parameter<double>(prefix + "geopoint.longitude", config_ptr->environment.geopoint.longitude);
	node->get_parameter<double>(prefix + "geopoint.altitude", config_ptr->environment.geopoint.altitude);

	node->get_parameter<float>(prefix + "wind_vel.x", config_ptr->environment.wind_vel.x());
	node->get_parameter<float>(prefix + "wind_vel.y", config_ptr->environment.wind_vel.y());
	node->get_parameter<float>(prefix + "wind_vel.z", config_ptr->environment.wind_vel.z());
}

void MissionSettingsParser::parseTargetParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
{
	// Get target ID list and iterate over them
	std::vector<std::string> target_ids;
	node->get_parameter<std::vector<std::string>>(prefix + "id_list", target_ids);
	for (const auto& target_id : target_ids)
	{
		auto target = std::make_shared<TargetConfig>();
		target->id = target_id;
		const std::string target_prefix = prefix + target_id + ".";

		node->get_parameter<std::string>(target_prefix + "name", target->name);
		node->get_parameter<std::string>(target_prefix + "target_group_id", target->target_group_id);
		node->get_parameter<int>(target_prefix + "target_index", target->target_index);

		int8_t type_int;
		node->get_parameter<int8_t>(target_prefix + "type", type_int);
		target->type = static_cast<TargetType>(type_int);

		node->get_parameter<int>(target_prefix + "count", target->count);

		int8_t priority_int;
		node->get_parameter<int8_t>(target_prefix + "priority", priority_int);
		target->priority = static_cast<Priority>(priority_int);

		node->get_parameter<std::string>(target_prefix + "trajectory_folder", target->trajectory_folder);

		config_ptr->target_group[target_id] = target;
	}
}

void MissionSettingsParser::parseAgentParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
{
	// Get agent ID list and iterate over them
	std::vector<std::string> agent_ids;
	node->get_parameter<std::vector<std::string>>(prefix + "id_list", agent_ids);
	for (const auto& agent_id : agent_ids)
	{
		auto agent = std::make_shared<AgentConfig>();
		agent->id = agent_id;
		const std::string agent_prefix = prefix + agent_id + ".";

		node->get_parameter<int>(agent_prefix + "idx", agent->idx);
		node->get_parameter<std::string>(agent_prefix + "name", agent->name);
		node->get_parameter<std::string>(agent_prefix + "agent_team_id", agent->agent_team_id);
		node->get_parameter<std::string>(agent_prefix + "tracking_id", agent->tracking_id);
		node->get_parameter<std::string>(agent_prefix + "drone_id", agent->drone_id);

		// Parse position (x, y, z)
		double pos_x, pos_y, pos_z;
		node->get_parameter<double>(agent_prefix + "position.x", pos_x);
		node->get_parameter<double>(agent_prefix + "position.y", pos_y);
		node->get_parameter<double>(agent_prefix + "position.z", pos_z);
		agent->position.x() = static_cast<float>(pos_x);
		agent->position.y() = static_cast<float>(pos_y);
		agent->position.z() = static_cast<float>(pos_z);

		// Parse orientation (roll, pitch, yaw)
		double roll, pitch, yaw;
		node->get_parameter<double>(agent_prefix + "orientation.roll", roll);
		node->get_parameter<double>(agent_prefix + "orientation.pitch", pitch);
		node->get_parameter<double>(agent_prefix + "orientation.yaw", yaw);
		agent->orientation.x() = static_cast<float>(roll);
		agent->orientation.y() = static_cast<float>(pitch);
		agent->orientation.z() = static_cast<float>(yaw);

		node->get_parameter<float>(agent_prefix + "safety_radius", agent->safety_radius);
		node->get_parameter<float>(agent_prefix + "max_altitude", agent->max_altitude);
		node->get_parameter<float>(agent_prefix + "battery_capacity", agent->battery_capacity);

		parseTrackingParameters(node, agent, agent_prefix + "tracking.");
		parseDroneParameters(node, agent, agent_prefix + "drone.");

		config_ptr->agent_team[agent_id] = agent;
	}
}

void MissionSettingsParser::parseTrackingParameters(const NodePtr& node, AgentConfigPtr& agent, const std::string& prefix)
{
	node->get_parameter<std::string>(prefix + "id", agent->tracking.id);
	node->get_parameter<std::string>(prefix + "name", agent->tracking.name);
	node->get_parameter<std::string>(prefix + "observation_set_id", agent->tracking.observation_set_id);
	node->get_parameter<float>(prefix + "min_target_size", agent->tracking.min_target_size);
	node->get_parameter<float>(prefix + "max_target_size", agent->tracking.max_target_size);
	node->get_parameter<float>(prefix + "ref_target_size", agent->tracking.ref_target_size);

	// Get multi camera ID list and iterate over them
	std::vector<std::string> multi_camera_ids;
	node->get_parameter_or<std::vector<std::string>>(prefix + "multi_cameras.ids", multi_camera_ids, std::vector<std::string>());
	for (const auto& multi_camera_id : multi_camera_ids)
	{
		auto multi_camera = std::make_shared<MultiCameraConfig>();
		multi_camera->id = multi_camera_id;
		parseMultiCameraParameters(node, multi_camera, prefix + "multi_cameras." + multi_camera_id + ".");
		agent->tracking.multi_camera_set[multi_camera_id] = multi_camera;
	}

	// Get multi window ID list and iterate over them
	std::vector<std::string> multi_window_ids;
	node->get_parameter_or<std::vector<std::string>>(prefix + "multi_windows.ids", multi_window_ids, std::vector<std::string>());
	for (const auto& multi_window_id : multi_window_ids)
	{
		auto multi_window = std::make_shared<MultiWindowConfig>();
		multi_window->id = multi_window_id;
		parseMultiWindowParameters(node, multi_window, prefix + "multi_windows." + multi_window_id + ".");
		agent->tracking.multi_window_set[multi_window_id] = multi_window;
	}
}

void MissionSettingsParser::parseMultiCameraParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix)
{
	node->get_parameter<std::string>(prefix + "name", multi_camera->name);
	node->get_parameter<std::string>(prefix + "observation_set_id", multi_camera->observation_set_id);
	node->get_parameter<std::string>(prefix + "camera_id", multi_camera->camera_id);
	node->get_parameter<std::string>(prefix + "gimbal_id", multi_camera->gimbal_id);

	int8_t role_int;
	node->get_parameter<int8_t>(prefix + "role", role_int);
	multi_camera->role = static_cast<ObservationRole>(role_int);

	// Parse position (x, y, z)
	double pos_x, pos_y, pos_z;
	node->get_parameter<double>(prefix + "position.x", pos_x);
	node->get_parameter<double>(prefix + "position.y", pos_y);
	node->get_parameter<double>(prefix + "position.z", pos_z);
	multi_camera->position.x() = static_cast<float>(pos_x);
	multi_camera->position.y() = static_cast<float>(pos_y);
	multi_camera->position.z() = static_cast<float>(pos_z);

	// Parse orientation (roll, pitch, yaw)
	double roll, pitch, yaw;
	node->get_parameter<double>(prefix + "orientation.roll", roll);
	node->get_parameter<double>(prefix + "orientation.pitch", pitch);
	node->get_parameter<double>(prefix + "orientation.yaw", yaw);
	multi_camera->orientation.x() = static_cast<float>(roll);
	multi_camera->orientation.y() = static_cast<float>(pitch);
	multi_camera->orientation.z() = static_cast<float>(yaw);

	node->get_parameter<float>(prefix + "min_focal", multi_camera->min_focal);
	node->get_parameter<float>(prefix + "max_focal", multi_camera->max_focal);
	node->get_parameter<float>(prefix + "ref_focal", multi_camera->ref_focal);

	node->get_parameter<std::string>(prefix + "source_stream_url", multi_camera->source_stream_url);

	parseCameraParameters(node, multi_camera, prefix + "camera.");
	parseGimbalParameters(node, multi_camera, prefix + "gimbal.");
}

void MissionSettingsParser::parseMultiWindowParameters(const NodePtr& node, MultiWindowConfigPtr& multi_window, const std::string& prefix)
{
	node->get_parameter<std::string>(prefix + "name", multi_window->name);
	node->get_parameter<std::string>(prefix + "observation_set_id", multi_window->observation_set_id);

	node->get_parameter<float>(prefix + "min_lambda", multi_window->min_lambda);
	node->get_parameter<float>(prefix + "max_lambda", multi_window->max_lambda);
	node->get_parameter<float>(prefix + "ref_lambda", multi_window->ref_lambda);
}

void MissionSettingsParser::parseCameraParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix)
{
	node->get_parameter<std::string>(prefix + "id", multi_camera->camera.id);
	node->get_parameter<std::string>(prefix + "name", multi_camera->camera.name);

	int8_t type_int;
	node->get_parameter<int8_t>(prefix + "type", type_int);
	multi_camera->camera.type = static_cast<CameraType>(type_int);

	// Parse resolution (width, height)
	int64_t width, height;
	node->get_parameter<int64_t>(prefix + "resolution.width", width);
	node->get_parameter<int64_t>(prefix + "resolution.height", height);
	multi_camera->camera.resolution(0) = static_cast<int>(width);
	multi_camera->camera.resolution(1) = static_cast<int>(height);

	// Parse sensor_size (width, height)
	double sensor_w, sensor_h;
	node->get_parameter<double>(prefix + "sensor_size.width", sensor_w);
	node->get_parameter<double>(prefix + "sensor_size.height", sensor_h);
	multi_camera->camera.sensor_size(0) = static_cast<float>(sensor_w);
	multi_camera->camera.sensor_size(1) = static_cast<float>(sensor_h);

	// Parse distortion (K1, K2, K3, P1, P2)
	double k1, k2, k3, p1, p2;
	node->get_parameter<double>(prefix + "distortion.K1", k1);
	node->get_parameter<double>(prefix + "distortion.K2", k2);
	node->get_parameter<double>(prefix + "distortion.K3", k3);
	node->get_parameter<double>(prefix + "distortion.P1", p1);
	node->get_parameter<double>(prefix + "distortion.P2", p2);
	multi_camera->camera.distortion.K1 = static_cast<float>(k1);
	multi_camera->camera.distortion.K2 = static_cast<float>(k2);
	multi_camera->camera.distortion.K3 = static_cast<float>(k3);
	multi_camera->camera.distortion.P1 = static_cast<float>(p1);
	multi_camera->camera.distortion.P2 = static_cast<float>(p2);

	node->get_parameter<bool>(prefix + "enable_sensor_noise", multi_camera->camera.enable_sensor_noise);

	// Parse sensor_noise (rand_contrib, rand_size, rand_speed)
	double rand_contrib, rand_size, rand_speed;
	node->get_parameter<double>(prefix + "sensor_noise.rand_contrib", rand_contrib);
	node->get_parameter<double>(prefix + "sensor_noise.rand_size", rand_size);
	node->get_parameter<double>(prefix + "sensor_noise.rand_speed", rand_speed);
	multi_camera->camera.sensor_noise.rand_contrib = static_cast<float>(rand_contrib);
	multi_camera->camera.sensor_noise.rand_size = static_cast<float>(rand_size);
	multi_camera->camera.sensor_noise.rand_speed = static_cast<float>(rand_speed);

	node->get_parameter<float>(prefix + "weight", multi_camera->camera.weight);
	node->get_parameter<float>(prefix + "idle_power", multi_camera->camera.idle_power);
	node->get_parameter<float>(prefix + "active_power", multi_camera->camera.active_power);
}

void MissionSettingsParser::parseGimbalParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix)
{
	node->get_parameter<std::string>(prefix + "id", multi_camera->gimbal.id);
	node->get_parameter<std::string>(prefix + "name", multi_camera->gimbal.name);
	node->get_parameter<bool>(prefix + "enable_roll", multi_camera->gimbal.enable_roll);

	// Parse roll (min_angle, max_angle, max_speed)
	double roll_min_angle, roll_max_angle, roll_max_speed;
	node->get_parameter<double>(prefix + "roll.min_angle", roll_min_angle);
	node->get_parameter<double>(prefix + "roll.max_angle", roll_max_angle);
	node->get_parameter<double>(prefix + "roll.max_speed", roll_max_speed);
	multi_camera->gimbal.roll.min_angle = static_cast<float>(roll_min_angle);
	multi_camera->gimbal.roll.max_angle = static_cast<float>(roll_max_angle);
	multi_camera->gimbal.roll.max_speed = static_cast<float>(roll_max_speed);

	node->get_parameter<bool>(prefix + "enable_pitch", multi_camera->gimbal.enable_pitch);

	// Parse pitch (min_angle, max_angle, max_speed)
	double pitch_min_angle, pitch_max_angle, pitch_max_speed;
	node->get_parameter<double>(prefix + "pitch.min_angle", pitch_min_angle);
	node->get_parameter<double>(prefix + "pitch.max_angle", pitch_max_angle);
	node->get_parameter<double>(prefix + "pitch.max_speed", pitch_max_speed);
	multi_camera->gimbal.pitch.min_angle = static_cast<float>(pitch_min_angle);
	multi_camera->gimbal.pitch.max_angle = static_cast<float>(pitch_max_angle);
	multi_camera->gimbal.pitch.max_speed = static_cast<float>(pitch_max_speed);

	node->get_parameter<bool>(prefix + "enable_yaw", multi_camera->gimbal.enable_yaw);

	// Parse yaw (min_angle, max_angle, max_speed)
	double yaw_min_angle, yaw_max_angle, yaw_max_speed;
	node->get_parameter<double>(prefix + "yaw.min_angle", yaw_min_angle);
	node->get_parameter<double>(prefix + "yaw.max_angle", yaw_max_angle);
	node->get_parameter<double>(prefix + "yaw.max_speed", yaw_max_speed);
	multi_camera->gimbal.yaw.min_angle = static_cast<float>(yaw_min_angle);
	multi_camera->gimbal.yaw.max_angle = static_cast<float>(yaw_max_angle);
	multi_camera->gimbal.yaw.max_speed = static_cast<float>(yaw_max_speed);

	node->get_parameter<float>(prefix + "weight", multi_camera->gimbal.weight);
	node->get_parameter<float>(prefix + "idle_power", multi_camera->gimbal.idle_power);
	node->get_parameter<float>(prefix + "active_power", multi_camera->gimbal.active_power);
}

void MissionSettingsParser::parseDroneParameters(const NodePtr& node, AgentConfigPtr& agent, const std::string& prefix)
{
	node->get_parameter<std::string>(prefix + "id", agent->drone.id);
	node->get_parameter<std::string>(prefix + "name", agent->drone.name);

	int8_t type_int;
	node->get_parameter<int8_t>(prefix + "type", type_int);
	agent->drone.type = static_cast<DroneType>(type_int);

	node->get_parameter<float>(prefix + "cruise_speed", agent->drone.cruise_speed);
	node->get_parameter<float>(prefix + "max_speed", agent->drone.max_speed);

	node->get_parameter<bool>(prefix + "enable_barometer", agent->drone.enable_barometer);
	// Parse barometer (white_noise_sigma)
	double baro_noise;
	node->get_parameter<double>(prefix + "barometer.white_noise_sigma", baro_noise);
	agent->drone.barometer.white_noise_sigma = static_cast<float>(baro_noise);

	node->get_parameter<bool>(prefix + "enable_imu", agent->drone.enable_imu);
	// Parse imu (angular_white_noise_sigma, velocity_white_noise_sigma)
	double imu_angular_noise, imu_velocity_noise;
	node->get_parameter<double>(prefix + "imu.angular_white_noise_sigma", imu_angular_noise);
	node->get_parameter<double>(prefix + "imu.velocity_white_noise_sigma", imu_velocity_noise);
	agent->drone.imu.angular_white_noise_sigma = static_cast<float>(imu_angular_noise);
	agent->drone.imu.velocity_white_noise_sigma = static_cast<float>(imu_velocity_noise);

	node->get_parameter<bool>(prefix + "enable_gps", agent->drone.enable_gps);
	// Parse gps (eph_initial, epv_initial, eph_final, epv_final)
	double eph_initial, epv_initial, eph_final, epv_final;
	node->get_parameter<double>(prefix + "gps.eph_initial", eph_initial);
	node->get_parameter<double>(prefix + "gps.epv_initial", epv_initial);
	node->get_parameter<double>(prefix + "gps.eph_final", eph_final);
	node->get_parameter<double>(prefix + "gps.epv_final", epv_final);
	agent->drone.gps.eph_initial = static_cast<float>(eph_initial);
	agent->drone.gps.epv_initial = static_cast<float>(epv_initial);
	agent->drone.gps.eph_final = static_cast<float>(eph_final);
	agent->drone.gps.epv_final = static_cast<float>(epv_final);

	node->get_parameter<bool>(prefix + "enable_magnetometer", agent->drone.enable_magnetometer);
	// Parse magnetometer (white_noise_sigma, white_noise_bias)
	double mag_noise_sigma, mag_noise_bias;
	node->get_parameter<double>(prefix + "magnetometer.white_noise_sigma", mag_noise_sigma);
	node->get_parameter<double>(prefix + "magnetometer.white_noise_bias", mag_noise_bias);
	agent->drone.magnetometer.white_noise_sigma = static_cast<float>(mag_noise_sigma);
	agent->drone.magnetometer.white_noise_bias = static_cast<float>(mag_noise_bias);

	node->get_parameter<float>(prefix + "base_weight", agent->drone.base_weight);
	node->get_parameter<float>(prefix + "max_payload_weight", agent->drone.max_payload_weight);
	node->get_parameter<float>(prefix + "hover_power", agent->drone.hover_power);
	node->get_parameter<float>(prefix + "cruise_power", agent->drone.cruise_power);
	node->get_parameter<float>(prefix + "load_factor", agent->drone.load_factor);
}

void MissionSettingsParser::parseSystemParameters(const NodePtr& node, MissionConfigPtr& config_ptr)
{
	// Simulation settings
	std::string simulation_framework_str;
	node->get_parameter<std::string>("simulation.framework", simulation_framework_str);
	config_ptr->system.simulation_framework = simulationFrameworkFromString(simulation_framework_str);
	node->get_parameter<float>("simulation.clock_speed", config_ptr->system.clock_speed);
	node->get_parameter<int>("simulation.quality_preset", config_ptr->system.quality_preset);

	// Path settings
	node->get_parameter<std::string>("path.config_spreadsheet_path", config_ptr->system.config_source_file);
	node->get_parameter<std::string>("path.airsim_settings_path", config_ptr->system.airsim_settings_destination_file);
	node->get_parameter<std::string>("path.trajectory_root", config_ptr->system.trajectory_root);

	// GUI settings
	// Scenario view settings
	node->get_parameter<ID>("gui.scenario_view_id", config_ptr->system.scenario_view_id);
	node->get_parameter<ID>("gui.scenario_camera_id", config_ptr->system.scenario_camera_id);
	std::vector<double> scenario_camera_position_vec;
	node->get_parameter<std::vector<double>>("gui.scenario_camera_position", scenario_camera_position_vec);
	if (scenario_camera_position_vec.size() >= 3)
	{
		config_ptr->system.scenario_camera_position.x() = scenario_camera_position_vec[0];
		config_ptr->system.scenario_camera_position.y() = scenario_camera_position_vec[1];
		config_ptr->system.scenario_camera_position.z() = scenario_camera_position_vec[2];
	}
	std::vector<double> scenario_camera_orientation_vec;
	node->get_parameter<std::vector<double>>("gui.scenario_camera_orientation", scenario_camera_orientation_vec);
	if (scenario_camera_orientation_vec.size() >= 3)
	{
		config_ptr->system.scenario_camera_orientation.x() = MathUtils::degToRad(scenario_camera_orientation_vec[0]);
		config_ptr->system.scenario_camera_orientation.y() = MathUtils::degToRad(scenario_camera_orientation_vec[1]);
		config_ptr->system.scenario_camera_orientation.z() = MathUtils::degToRad(scenario_camera_orientation_vec[2]);
	}
	std::vector<int64_t> scenario_camera_resolution_vec;
	node->get_parameter<std::vector<int64_t>>("gui.scenario_camera_resolution", scenario_camera_resolution_vec);
	if (scenario_camera_resolution_vec.size() >= 2)
	{
		config_ptr->system.scenario_camera_resolution(0) = static_cast<int>(scenario_camera_resolution_vec[0]);
		config_ptr->system.scenario_camera_resolution(1) = static_cast<int>(scenario_camera_resolution_vec[1]);
	}
	// Agent view settings
	node->get_parameter<ID>("gui.agent_view_id", config_ptr->system.agent_view_id);
	node->get_parameter<ID>("gui.agent_camera_id", config_ptr->system.agent_camera_id);
	std::vector<double> agent_camera_position_vec;
	node->get_parameter<std::vector<double>>("gui.agent_camera_position", agent_camera_position_vec);
	if (agent_camera_position_vec.size() >= 3)
	{
		config_ptr->system.agent_camera_position.x() = agent_camera_position_vec[0];
		config_ptr->system.agent_camera_position.y() = agent_camera_position_vec[1];
		config_ptr->system.agent_camera_position.z() = agent_camera_position_vec[2];
	}
	std::vector<double> agent_camera_orientation_vec;
	node->get_parameter<std::vector<double>>("gui.agent_camera_orientation", agent_camera_orientation_vec);
	if (agent_camera_orientation_vec.size() >= 3)
	{
		config_ptr->system.agent_camera_orientation.x() = MathUtils::degToRad(agent_camera_orientation_vec[0]);
		config_ptr->system.agent_camera_orientation.y() = MathUtils::degToRad(agent_camera_orientation_vec[1]);
		config_ptr->system.agent_camera_orientation.z() = MathUtils::degToRad(agent_camera_orientation_vec[2]);
	}
	std::vector<int64_t> agent_camera_resolution_vec;
	node->get_parameter<std::vector<int64_t>>("gui.agent_camera_resolution", agent_camera_resolution_vec);
	if (agent_camera_resolution_vec.size() >= 2)
	{
		config_ptr->system.agent_camera_resolution(0) = static_cast<int>(agent_camera_resolution_vec[0]);
		config_ptr->system.agent_camera_resolution(1) = static_cast<int>(agent_camera_resolution_vec[1]);
	}
	// Payload view settings
	node->get_parameter<ID>("gui.payload_view_id", config_ptr->system.payload_view_id);
	node->get_parameter<ID>("gui.payload_camera_id", config_ptr->system.payload_camera_id);
	std::vector<double> payload_camera_position_vec;
	node->get_parameter<std::vector<double>>("gui.payload_camera_position", payload_camera_position_vec);
	if (payload_camera_position_vec.size() >= 3)
	{
		config_ptr->system.payload_camera_position.x() = payload_camera_position_vec[0];
		config_ptr->system.payload_camera_position.y() = payload_camera_position_vec[1];
		config_ptr->system.payload_camera_position.z() = payload_camera_position_vec[2];
	}
	std::vector<double> payload_camera_orientation_vec;
	node->get_parameter<std::vector<double>>("gui.payload_camera_orientation", payload_camera_orientation_vec);
	if (payload_camera_orientation_vec.size() >= 3)
	{
		config_ptr->system.payload_camera_orientation.x() = MathUtils::degToRad(payload_camera_orientation_vec[0]);
		config_ptr->system.payload_camera_orientation.y() = MathUtils::degToRad(payload_camera_orientation_vec[1]);
		config_ptr->system.payload_camera_orientation.z() = MathUtils::degToRad(payload_camera_orientation_vec[2]);
	}
	std::vector<int64_t> payload_camera_resolution_vec;
	node->get_parameter<std::vector<int64_t>>("gui.payload_camera_resolution", payload_camera_resolution_vec);
	if (payload_camera_resolution_vec.size() >= 2)
	{
		config_ptr->system.payload_camera_resolution(0) = static_cast<int>(payload_camera_resolution_vec[0]);
		config_ptr->system.payload_camera_resolution(1) = static_cast<int>(payload_camera_resolution_vec[1]);
	}
}

void MissionSettingsParser::parseTopicParameters(const NodePtr& node, MissionConfigPtr& config_ptr)
{
	// Coordinator topics
	node->get_parameter<std::string>("coordinator_topics.registration", config_ptr->topics.registration);
	node->get_parameter<std::string>("coordinator_topics.mission_status", config_ptr->topics.mission_status);
	node->get_parameter<std::string>("coordinator_topics.fleet_status", config_ptr->topics.fleet_status);
	node->get_parameter<std::string>("coordinator_topics.global_origin", config_ptr->topics.global_origin);
	node->get_parameter<std::string>("coordinator_topics.target_position", config_ptr->topics.target_position);
	node->get_parameter<std::string>("coordinator_topics.cluster_assignment", config_ptr->topics.cluster_assignment);
	node->get_parameter<std::string>("coordinator_topics.cluster_geometry", config_ptr->topics.cluster_geometry);
	node->get_parameter<std::string>("coordinator_topics.agent_assignment", config_ptr->topics.agent_assignment);
	node->get_parameter<std::string>("coordinator_topics.agent_clusters", config_ptr->topics.agent_clusters);
	node->get_parameter<std::string>("coordinator_topics.assignment_solve_duration", config_ptr->topics.assignment_solve_duration);
	node->get_parameter<std::string>("coordinator_topics.assignment_node_count", config_ptr->topics.assignment_node_count);
	node->get_parameter<std::string>("coordinator_topics.assignment_swap_count", config_ptr->topics.assignment_swap_count);

	// Agent topics
	node->get_parameter<std::string>("agent_topics.agent_status", config_ptr->topics.agent_status);
	node->get_parameter<std::string>("agent_topics.agent_global_position", config_ptr->topics.agent_global_position);
	node->get_parameter<std::string>("agent_topics.agent_local_position", config_ptr->topics.agent_local_position);
	node->get_parameter<std::string>("agent_topics.agent_position_setpoint", config_ptr->topics.agent_position_setpoint);
	node->get_parameter<std::string>("agent_topics.position_solve_duration", config_ptr->topics.position_solve_duration);
	node->get_parameter<std::string>("agent_topics.observation_setpoints", config_ptr->topics.observation_setpoints);
	node->get_parameter<std::string>("agent_topics.image", config_ptr->topics.image);
	node->get_parameter<std::string>("agent_topics.image_compressed", config_ptr->topics.image_compressed);
	node->get_parameter<std::string>("agent_topics.camera_info", config_ptr->topics.camera_info);

	// Simulation topics
	node->get_parameter<std::string>("simulation_topics.simulation_image", config_ptr->topics.simulation_image);

	// Operator topics
	node->get_parameter<std::string>("operator_topics.annotations", config_ptr->topics.annotations);
	node->get_parameter<std::string>("operator_topics.scene", config_ptr->topics.scene);
	node->get_parameter<std::string>("operator_topics.start_mission", config_ptr->topics.start_mission);
	node->get_parameter<std::string>("operator_topics.pause_mission", config_ptr->topics.pause_mission);
	node->get_parameter<std::string>("operator_topics.abort_mission", config_ptr->topics.abort_mission);
	node->get_parameter<std::string>("operator_topics.arm_all", config_ptr->topics.arm_all);
	node->get_parameter<std::string>("operator_topics.land_all", config_ptr->topics.land_all);
	node->get_parameter<std::string>("operator_topics.return_home", config_ptr->topics.return_home);
	node->get_parameter<std::string>("operator_topics.mission_metrics", config_ptr->topics.mission_metrics);
	node->get_parameter<std::string>("operator_topics.fleet_metrics", config_ptr->topics.fleet_metrics);
	node->get_parameter<std::string>("operator_topics.agent_metrics", config_ptr->topics.agent_metrics);
	node->get_parameter<std::string>("operator_topics.target_metrics", config_ptr->topics.target_metrics);
	node->get_parameter<std::string>("operator_topics.cluster_metrics", config_ptr->topics.cluster_metrics);
}

void MissionSettingsParser::parseFrameParameters(const NodePtr& node, MissionConfigPtr& config_ptr)
{
	// Global frames
	node->get_parameter<std::string>("global_frames.world", config_ptr->frames.world);

	// Agent frames
	node->get_parameter<std::string>("agent_frames.agent_local", config_ptr->frames.agent_local);
	node->get_parameter<std::string>("agent_frames.agent_body", config_ptr->frames.agent_body);
	node->get_parameter<std::string>("agent_frames.camera_body", config_ptr->frames.camera_body);
	node->get_parameter<std::string>("agent_frames.camera_optical", config_ptr->frames.camera_optical);
}