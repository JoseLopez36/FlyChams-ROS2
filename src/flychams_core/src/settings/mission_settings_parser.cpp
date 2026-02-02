#include "flychams_core/settings/mission_settings_parser.hpp"

namespace flychams::core
{
	void MissionSettingsParser::parseMissionParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
	{
		config_ptr->id = RosUtils::getParameter<std::string>(node, prefix + "id");
		config_ptr->name = RosUtils::getParameter<std::string>(node, prefix + "name");

		config_ptr->environment_id = RosUtils::getParameter<std::string>(node, prefix + "environment_id");
		config_ptr->target_group_id = RosUtils::getParameter<std::string>(node, prefix + "target_group_id");
		config_ptr->agent_team_id = RosUtils::getParameter<std::string>(node, prefix + "agent_team_id"); 

		config_ptr->horizontal_constraint(0) = RosUtils::getParameter<float>(node, prefix + "horizontal_constraint.min");
		config_ptr->horizontal_constraint(1) = RosUtils::getParameter<float>(node, prefix + "horizontal_constraint.max");

		config_ptr->vertical_constraint(0) = RosUtils::getParameter<float>(node, prefix + "vertical_constraint.min");
		config_ptr->vertical_constraint(1) = RosUtils::getParameter<float>(node, prefix + "vertical_constraint.max");
		
		const int8_t autopilot_int = RosUtils::getParameter<int8_t>(node, prefix + "autopilot");
		config_ptr->autopilot = static_cast<Autopilot>(autopilot_int);
		
		config_ptr->start_date.year = RosUtils::getParameter<int64_t>(node, prefix + "start_date.year");
		config_ptr->start_date.month = RosUtils::getParameter<int64_t>(node, prefix + "start_date.month");
		config_ptr->start_date.day = RosUtils::getParameter<int64_t>(node, prefix + "start_date.day");
		
		config_ptr->start_hour.hours = RosUtils::getParameter<int64_t>(node, prefix + "start_hour.hours");
		config_ptr->start_hour.minutes = RosUtils::getParameter<int64_t>(node, prefix + "start_hour.minutes");
		config_ptr->start_hour.seconds = RosUtils::getParameter<int64_t>(node, prefix + "start_hour.seconds");
	}

	void MissionSettingsParser::parseEnvironmentParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
	{
		config_ptr->environment.id = RosUtils::getParameter<std::string>(node, prefix + "id");
		config_ptr->environment.name = RosUtils::getParameter<std::string>(node, prefix + "name");
		
		config_ptr->environment.geopoint.latitude = RosUtils::getParameter<double>(node, prefix + "geopoint.latitude");
		config_ptr->environment.geopoint.longitude = RosUtils::getParameter<double>(node, prefix + "geopoint.longitude");
		config_ptr->environment.geopoint.altitude = RosUtils::getParameter<double>(node, prefix + "geopoint.altitude");
		
		config_ptr->environment.wind_vel.x() = RosUtils::getParameter<float>(node, prefix + "wind_vel.x");
		config_ptr->environment.wind_vel.y() = RosUtils::getParameter<float>(node, prefix + "wind_vel.y");
		config_ptr->environment.wind_vel.z() = RosUtils::getParameter<float>(node, prefix + "wind_vel.z");
	}

	void MissionSettingsParser::parseTargetParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
	{
		// Get target ID list and iterate over them
		std::vector<std::string> target_ids = RosUtils::getParameter<std::vector<std::string>>(node, prefix + "id_list");
		for (const auto& target_id : target_ids)
		{
			auto target = std::make_shared<TargetConfig>();
			target->id = target_id;
			const std::string target_prefix = prefix + target_id + ".";
			
			target->name = RosUtils::getParameter<std::string>(node, target_prefix + "name");
			target->target_group_id = RosUtils::getParameter<std::string>(node, target_prefix + "target_group_id");
			target->target_index = RosUtils::getParameter<int>(node, target_prefix + "target_index");
			
			const int8_t type_int = RosUtils::getParameter<int8_t>(node, target_prefix + "type");
			target->type = static_cast<TargetType>(type_int);
			
			target->count = RosUtils::getParameter<int>(node, target_prefix + "count");
			
			const int8_t priority_int = RosUtils::getParameter<int8_t>(node, target_prefix + "priority");
			target->priority = static_cast<Priority>(priority_int);
			
			target->trajectory_folder = RosUtils::getParameter<std::string>(node, target_prefix + "trajectory_folder");
			
			config_ptr->target_group[target_id] = target;
		}
	}

	void MissionSettingsParser::parseAgentParameters(const NodePtr& node, const std::string& prefix, MissionConfigPtr& config_ptr)
	{
		// Get agent ID list and iterate over them
		std::vector<std::string> agent_ids = RosUtils::getParameter<std::vector<std::string>>(node, prefix + "id_list");
		for (const auto& agent_id : agent_ids)
		{
			auto agent = std::make_shared<AgentConfig>();
			agent->id = agent_id;
			const std::string agent_prefix = prefix + agent_id + ".";
			
			agent->name = RosUtils::getParameter<std::string>(node, agent_prefix + "name");
			agent->agent_team_id = RosUtils::getParameter<std::string>(node, agent_prefix + "agent_team_id");
			agent->tracking_id = RosUtils::getParameter<std::string>(node, agent_prefix + "tracking_id");
			agent->drone_id = RosUtils::getParameter<std::string>(node, agent_prefix + "drone_id");
			
			// Parse position (x, y, z)
			agent->position.x() = RosUtils::getParameter<double>(node, agent_prefix + "position.x");
			agent->position.y() = RosUtils::getParameter<double>(node, agent_prefix + "position.y");
			agent->position.z() = RosUtils::getParameter<double>(node, agent_prefix + "position.z");
			
			// Parse orientation (roll, pitch, yaw)
			agent->orientation.x() = RosUtils::getParameter<double>(node, agent_prefix + "orientation.roll");
			agent->orientation.y() = RosUtils::getParameter<double>(node, agent_prefix + "orientation.pitch");
			agent->orientation.z() = RosUtils::getParameter<double>(node, agent_prefix + "orientation.yaw");
			
			agent->safety_radius = RosUtils::getParameter<float>(node, agent_prefix + "safety_radius");
			agent->max_altitude = RosUtils::getParameter<float>(node, agent_prefix + "max_altitude");
			agent->battery_capacity = RosUtils::getParameter<float>(node, agent_prefix + "battery_capacity");
			
			parseTrackingParameters(node, agent, agent_prefix + "tracking.");
			parseDroneParameters(node, agent, agent_prefix + "drone.");
			
			config_ptr->agent_team[agent_id] = agent;
		}
	}

	void MissionSettingsParser::parseTrackingParameters(const NodePtr& node, AgentConfigPtr& agent, const std::string& prefix)
	{
		agent->tracking.id = RosUtils::getParameter<std::string>(node, prefix + "id");
		agent->tracking.name = RosUtils::getParameter<std::string>(node, prefix + "name");
		agent->tracking.observation_set_id = RosUtils::getParameter<std::string>(node, prefix + "observation_set_id");
		agent->tracking.min_target_size = RosUtils::getParameter<float>(node, prefix + "min_target_size");
		agent->tracking.max_target_size = RosUtils::getParameter<float>(node, prefix + "max_target_size");
		agent->tracking.ref_target_size = RosUtils::getParameter<float>(node, prefix + "ref_target_size");
		
		// Get multi camera ID list and iterate over them
		std::vector<std::string> multi_camera_ids = RosUtils::getParameterOr<std::vector<std::string>>(node, prefix + "multi_cameras.ids", std::vector<std::string>());
		for (const auto& multi_camera_id : multi_camera_ids)
		{
			auto multi_camera = std::make_shared<MultiCameraConfig>();
			multi_camera->id = multi_camera_id;
			parseMultiCameraParameters(node, multi_camera, prefix + "multi_cameras." + multi_camera_id + ".");
			agent->tracking.multi_camera_set[multi_camera_id] = multi_camera;
		}
		
		// Get multi window ID list and iterate over them
		std::vector<std::string> multi_window_ids = RosUtils::getParameterOr<std::vector<std::string>>(node, prefix + "multi_windows.ids", std::vector<std::string>());
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
		multi_camera->name = RosUtils::getParameter<std::string>(node, prefix + "name");
		multi_camera->observation_set_id = RosUtils::getParameter<std::string>(node, prefix + "observation_set_id");
		multi_camera->camera_id = RosUtils::getParameter<std::string>(node, prefix + "camera_id");
		multi_camera->gimbal_id = RosUtils::getParameter<std::string>(node, prefix + "gimbal_id");
		
		const int8_t role_int = RosUtils::getParameter<int8_t>(node, prefix + "role");
		multi_camera->role = static_cast<ObservationRole>(role_int);
		
		// Parse position (x, y, z)
		multi_camera->position.x() = RosUtils::getParameter<double>(node, prefix + "position.x");
		multi_camera->position.y() = RosUtils::getParameter<double>(node, prefix + "position.y");
		multi_camera->position.z() = RosUtils::getParameter<double>(node, prefix + "position.z");
		
		// Parse orientation (roll, pitch, yaw)
		multi_camera->orientation.x() = RosUtils::getParameter<double>(node, prefix + "orientation.roll");
		multi_camera->orientation.y() = RosUtils::getParameter<double>(node, prefix + "orientation.pitch");
		multi_camera->orientation.z() = RosUtils::getParameter<double>(node, prefix + "orientation.yaw");
		
		multi_camera->min_focal = RosUtils::getParameter<float>(node, prefix + "min_focal");
		multi_camera->max_focal = RosUtils::getParameter<float>(node, prefix + "max_focal");
		multi_camera->ref_focal = RosUtils::getParameter<float>(node, prefix + "ref_focal");
		multi_camera->src_stream_url = RosUtils::getParameter<std::string>(node, prefix + "src_stream_url");
		multi_camera->dst_stream_url = RosUtils::getParameter<std::string>(node, prefix + "dst_stream_url");
		multi_camera->hardware = RosUtils::getParameter<std::string>(node, prefix + "hardware");
		multi_camera->ip = RosUtils::getParameter<std::string>(node, prefix + "ip");
		multi_camera->port = RosUtils::getParameter<int>(node, prefix + "port");
		
		parseCameraParameters(node, multi_camera, prefix + "camera.");
		parseGimbalParameters(node, multi_camera, prefix + "gimbal.");
	}

	void MissionSettingsParser::parseMultiWindowParameters(const NodePtr& node, MultiWindowConfigPtr& multi_window, const std::string& prefix)
	{
		multi_window->name = RosUtils::getParameter<std::string>(node, prefix + "name");
		multi_window->observation_set_id = RosUtils::getParameter<std::string>(node, prefix + "observation_set_id");
		
		// Parse resolution (width, height)
		multi_window->resolution(0) = RosUtils::getParameter<int64_t>(node, prefix + "resolution.width");
		multi_window->resolution(1) = RosUtils::getParameter<int64_t>(node, prefix + "resolution.height");
		
		multi_window->min_lambda = RosUtils::getParameter<float>(node, prefix + "min_lambda");
		multi_window->max_lambda = RosUtils::getParameter<float>(node, prefix + "max_lambda");
		multi_window->ref_lambda = RosUtils::getParameter<float>(node, prefix + "ref_lambda");
		multi_window->dst_stream_url = RosUtils::getParameter<std::string>(node, prefix + "dst_stream_url");
	}

	void MissionSettingsParser::parseCameraParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix)
	{
		multi_camera->camera.id = RosUtils::getParameter<std::string>(node, prefix + "id");
		multi_camera->camera.name = RosUtils::getParameter<std::string>(node, prefix + "name");
		
		const int8_t type_int = RosUtils::getParameter<int8_t>(node, prefix + "type");
		multi_camera->camera.type = static_cast<CameraType>(type_int);
		
		// Parse resolution (width, height)
		multi_camera->camera.resolution(0) = RosUtils::getParameter<int64_t>(node, prefix + "resolution.width");
		multi_camera->camera.resolution(1) = RosUtils::getParameter<int64_t>(node, prefix + "resolution.height");
		
		// Parse sensor_size (width, height)
		multi_camera->camera.sensor_size(0) = RosUtils::getParameter<double>(node, prefix + "sensor_size.width");
		multi_camera->camera.sensor_size(1) = RosUtils::getParameter<double>(node, prefix + "sensor_size.height");
		
		// Parse distortion (K1, K2, K3, P1, P2)
		multi_camera->camera.distortion.K1 = RosUtils::getParameter<double>(node, prefix + "distortion.K1");
		multi_camera->camera.distortion.K2 = RosUtils::getParameter<double>(node, prefix + "distortion.K2");
		multi_camera->camera.distortion.K3 = RosUtils::getParameter<double>(node, prefix + "distortion.K3");
		multi_camera->camera.distortion.P1 = RosUtils::getParameter<double>(node, prefix + "distortion.P1");
		multi_camera->camera.distortion.P2 = RosUtils::getParameter<double>(node, prefix + "distortion.P2");
		
		multi_camera->camera.enable_sensor_noise = RosUtils::getParameter<bool>(node, prefix + "enable_sensor_noise");
		
		// Parse sensor_noise (rand_contrib, rand_size, rand_speed)
		multi_camera->camera.sensor_noise.rand_contrib = RosUtils::getParameter<double>(node, prefix + "sensor_noise.rand_contrib");
		multi_camera->camera.sensor_noise.rand_size = RosUtils::getParameter<double>(node, prefix + "sensor_noise.rand_size");
		multi_camera->camera.sensor_noise.rand_speed = RosUtils::getParameter<double>(node, prefix + "sensor_noise.rand_speed");
		
		multi_camera->camera.weight = RosUtils::getParameter<float>(node, prefix + "weight");
		multi_camera->camera.idle_power = RosUtils::getParameter<float>(node, prefix + "idle_power");
		multi_camera->camera.active_power = RosUtils::getParameter<float>(node, prefix + "active_power");
	}

	void MissionSettingsParser::parseGimbalParameters(const NodePtr& node, MultiCameraConfigPtr& multi_camera, const std::string& prefix)
	{
		multi_camera->gimbal.id = RosUtils::getParameter<std::string>(node, prefix + "id");
		multi_camera->gimbal.name = RosUtils::getParameter<std::string>(node, prefix + "name");
		multi_camera->gimbal.enable_roll = RosUtils::getParameter<bool>(node, prefix + "enable_roll");
		
		// Parse roll (min_angle, max_angle, max_speed)
		multi_camera->gimbal.roll.min_angle = RosUtils::getParameter<double>(node, prefix + "roll.min_angle");
		multi_camera->gimbal.roll.max_angle = RosUtils::getParameter<double>(node, prefix + "roll.max_angle");
		multi_camera->gimbal.roll.max_speed = RosUtils::getParameter<double>(node, prefix + "roll.max_speed");
		
		multi_camera->gimbal.enable_pitch = RosUtils::getParameter<bool>(node, prefix + "enable_pitch");
		
		// Parse pitch (min_angle, max_angle, max_speed)
		multi_camera->gimbal.pitch.min_angle = RosUtils::getParameter<double>(node, prefix + "pitch.min_angle");
		multi_camera->gimbal.pitch.max_angle = RosUtils::getParameter<double>(node, prefix + "pitch.max_angle");
		multi_camera->gimbal.pitch.max_speed = RosUtils::getParameter<double>(node, prefix + "pitch.max_speed");
		
		multi_camera->gimbal.enable_yaw = RosUtils::getParameter<bool>(node, prefix + "enable_yaw");
		
		// Parse yaw (min_angle, max_angle, max_speed)
		multi_camera->gimbal.yaw.min_angle = RosUtils::getParameter<double>(node, prefix + "yaw.min_angle");
		multi_camera->gimbal.yaw.max_angle = RosUtils::getParameter<double>(node, prefix + "yaw.max_angle");
		multi_camera->gimbal.yaw.max_speed = RosUtils::getParameter<double>(node, prefix + "yaw.max_speed");
		
		multi_camera->gimbal.weight = RosUtils::getParameter<float>(node, prefix + "weight");
		multi_camera->gimbal.idle_power = RosUtils::getParameter<float>(node, prefix + "idle_power");
		multi_camera->gimbal.active_power = RosUtils::getParameter<float>(node, prefix + "active_power");
	}

	void MissionSettingsParser::parseDroneParameters(const NodePtr& node, AgentConfigPtr& agent, const std::string& prefix)
	{
		agent->drone.id = RosUtils::getParameter<std::string>(node, prefix + "id");
		agent->drone.name = RosUtils::getParameter<std::string>(node, prefix + "name");
		
		const int8_t type_int = RosUtils::getParameter<int8_t>(node, prefix + "type");
		agent->drone.type = static_cast<DroneType>(type_int);
		
		agent->drone.cruise_speed = RosUtils::getParameter<float>(node, prefix + "cruise_speed");
		agent->drone.max_speed = RosUtils::getParameter<float>(node, prefix + "max_speed");
		
		agent->drone.enable_barometer = RosUtils::getParameter<bool>(node, prefix + "enable_barometer");
		// Parse barometer (white_noise_sigma)
		agent->drone.barometer.white_noise_sigma = RosUtils::getParameter<double>(node, prefix + "barometer.white_noise_sigma");
		
		agent->drone.enable_imu = RosUtils::getParameter<bool>(node, prefix + "enable_imu");
		// Parse imu (angular_white_noise_sigma, velocity_white_noise_sigma)
		agent->drone.imu.angular_white_noise_sigma = RosUtils::getParameter<double>(node, prefix + "imu.angular_white_noise_sigma");
		agent->drone.imu.velocity_white_noise_sigma = RosUtils::getParameter<double>(node, prefix + "imu.velocity_white_noise_sigma");
		
		agent->drone.enable_gps = RosUtils::getParameter<bool>(node, prefix + "enable_gps");
		// Parse gps (eph_initial, epv_initial, eph_final, epv_final)
		agent->drone.gps.eph_initial = RosUtils::getParameter<double>(node, prefix + "gps.eph_initial");
		agent->drone.gps.epv_initial = RosUtils::getParameter<double>(node, prefix + "gps.epv_initial");
		agent->drone.gps.eph_final = RosUtils::getParameter<double>(node, prefix + "gps.eph_final");
		agent->drone.gps.epv_final = RosUtils::getParameter<double>(node, prefix + "gps.epv_final");
		
		agent->drone.enable_magnetometer = RosUtils::getParameter<bool>(node, prefix + "enable_magnetometer");
		// Parse magnetometer (white_noise_sigma, white_noise_bias)
		agent->drone.magnetometer.white_noise_sigma = RosUtils::getParameter<double>(node, prefix + "magnetometer.white_noise_sigma");
		agent->drone.magnetometer.white_noise_bias = RosUtils::getParameter<double>(node, prefix + "magnetometer.white_noise_bias");
		
		agent->drone.base_weight = RosUtils::getParameter<float>(node, prefix + "base_weight");
		agent->drone.max_payload_weight = RosUtils::getParameter<float>(node, prefix + "max_payload_weight");
		agent->drone.hover_power = RosUtils::getParameter<float>(node, prefix + "hover_power");
		agent->drone.cruise_power = RosUtils::getParameter<float>(node, prefix + "cruise_power");
		agent->drone.load_factor = RosUtils::getParameter<float>(node, prefix + "load_factor");
	}

	void MissionSettingsParser::parseSystemParameters(const NodePtr& node, MissionConfigPtr& config_ptr)
	{
		// Simulation settings
		const std::string simulation_framework_str = RosUtils::getParameter<std::string>(node, "simulation.framework");
		config_ptr->system.simulation_framework = simulationFrameworkFromString(simulation_framework_str);
		config_ptr->system.clock_speed = RosUtils::getParameter<float>(node, "simulation.clock_speed");

		// Path settings
		config_ptr->system.config_source_file = RosUtils::getParameter<std::string>(node, "path.config_spreadsheet_path");
		config_ptr->system.airsim_settings_destination_file = RosUtils::getParameter<std::string>(node, "path.airsim_settings_path");
		config_ptr->system.trajectory_root = RosUtils::getParameter<std::string>(node, "path.trajectory_root");

		// GUI settings
		// Scenario view settings
		config_ptr->system.scenario_view_id = RosUtils::getParameter<ID>(node, "gui.scenario_view_id");
		config_ptr->system.scenario_camera_id = RosUtils::getParameter<ID>(node, "gui.scenario_camera_id");
		const std::vector<double> scenario_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.scenario_camera_position");
		if (scenario_camera_position_vec.size() >= 3)
		{
			config_ptr->system.scenario_camera_position.x() = scenario_camera_position_vec[0];
			config_ptr->system.scenario_camera_position.y() = scenario_camera_position_vec[1];
			config_ptr->system.scenario_camera_position.z() = scenario_camera_position_vec[2];
		}
		const std::vector<double> scenario_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.scenario_camera_orientation");
		if (scenario_camera_orientation_vec.size() >= 3)
		{
			config_ptr->system.scenario_camera_orientation.x() = MathUtils::degToRad(scenario_camera_orientation_vec[0]);
			config_ptr->system.scenario_camera_orientation.y() = MathUtils::degToRad(scenario_camera_orientation_vec[1]);
			config_ptr->system.scenario_camera_orientation.z() = MathUtils::degToRad(scenario_camera_orientation_vec[2]);
		}
		// Agent view settings
		config_ptr->system.agent_view_id = RosUtils::getParameter<ID>(node, "gui.agent_view_id");
		config_ptr->system.agent_camera_id = RosUtils::getParameter<ID>(node, "gui.agent_camera_id");
		const std::vector<double> agent_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.agent_camera_position");
		if (agent_camera_position_vec.size() >= 3)
		{
			config_ptr->system.agent_camera_position.x() = agent_camera_position_vec[0];
			config_ptr->system.agent_camera_position.y() = agent_camera_position_vec[1];
			config_ptr->system.agent_camera_position.z() = agent_camera_position_vec[2];
		}
		const std::vector<double> agent_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.agent_camera_orientation");
		if (agent_camera_orientation_vec.size() >= 3)
		{
			config_ptr->system.agent_camera_orientation.x() = MathUtils::degToRad(agent_camera_orientation_vec[0]);
			config_ptr->system.agent_camera_orientation.y() = MathUtils::degToRad(agent_camera_orientation_vec[1]);
			config_ptr->system.agent_camera_orientation.z() = MathUtils::degToRad(agent_camera_orientation_vec[2]);
		}
		// Payload view settings
		config_ptr->system.payload_view_id = RosUtils::getParameter<ID>(node, "gui.payload_view_id");
		config_ptr->system.payload_camera_id = RosUtils::getParameter<ID>(node, "gui.payload_camera_id");
		const std::vector<double> payload_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.payload_camera_position");
		if (payload_camera_position_vec.size() >= 3)
		{
			config_ptr->system.payload_camera_position.x() = payload_camera_position_vec[0];
			config_ptr->system.payload_camera_position.y() = payload_camera_position_vec[1];
			config_ptr->system.payload_camera_position.z() = payload_camera_position_vec[2];
		}
		const std::vector<double> payload_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.payload_camera_orientation");
		if (payload_camera_orientation_vec.size() >= 3)
		{
			config_ptr->system.payload_camera_orientation.x() = MathUtils::degToRad(payload_camera_orientation_vec[0]);
			config_ptr->system.payload_camera_orientation.y() = MathUtils::degToRad(payload_camera_orientation_vec[1]);
			config_ptr->system.payload_camera_orientation.z() = MathUtils::degToRad(payload_camera_orientation_vec[2]);
		}
		// Map view settings
		config_ptr->system.map_view_id = RosUtils::getParameter<ID>(node, "gui.map_view_id");
		config_ptr->system.map_camera_id = RosUtils::getParameter<ID>(node, "gui.map_camera_id");
		const std::vector<double> map_camera_position_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.map_camera_position");
		if (map_camera_position_vec.size() >= 3)
		{
			config_ptr->system.map_camera_position.x() = map_camera_position_vec[0];
			config_ptr->system.map_camera_position.y() = map_camera_position_vec[1];
			config_ptr->system.map_camera_position.z() = map_camera_position_vec[2];
		}
		const std::vector<double> map_camera_orientation_vec = RosUtils::getParameter<std::vector<double>>(node, "gui.map_camera_orientation");
		if (map_camera_orientation_vec.size() >= 3)
		{
			config_ptr->system.map_camera_orientation.x() = MathUtils::degToRad(map_camera_orientation_vec[0]);
			config_ptr->system.map_camera_orientation.y() = MathUtils::degToRad(map_camera_orientation_vec[1]);
			config_ptr->system.map_camera_orientation.z() = MathUtils::degToRad(map_camera_orientation_vec[2]);
		}
		// Tracking views settings
		config_ptr->system.tracking_view_ids = RosUtils::getParameter<std::vector<ID>>(node, "gui.tracking_view_ids");
	}

	void MissionSettingsParser::parseTopicParameters(const NodePtr& node, MissionConfigPtr& config_ptr)
	{
		// Global topics
		config_ptr->topics.registration = RosUtils::getParameter<std::string>(node, "global_topics.registration");
		config_ptr->topics.global_origin = RosUtils::getParameter<std::string>(node, "global_topics.global_origin");
		config_ptr->topics.global_metrics = RosUtils::getParameter<std::string>(node, "global_topics.metrics");

		// Agent topics
		config_ptr->topics.agent_status = RosUtils::getParameter<std::string>(node, "agent_topics.status");
		config_ptr->topics.agent_local_position = RosUtils::getParameter<std::string>(node, "agent_topics.local_position");
		config_ptr->topics.agent_global_position = RosUtils::getParameter<std::string>(node, "agent_topics.global_position");
		config_ptr->topics.agent_assignment = RosUtils::getParameter<std::string>(node, "agent_topics.assignment");
		config_ptr->topics.agent_clusters = RosUtils::getParameter<std::string>(node, "agent_topics.clusters");
		config_ptr->topics.agent_position_setpoint = RosUtils::getParameter<std::string>(node, "agent_topics.position_setpoint");
		config_ptr->topics.agent_optimization_duration = RosUtils::getParameter<std::string>(node, "agent_topics.optimization_duration");
		config_ptr->topics.agent_observation_setpoints = RosUtils::getParameter<std::string>(node, "agent_topics.observation_setpoints");
		config_ptr->topics.agent_gui_setpoints = RosUtils::getParameter<std::string>(node, "agent_topics.gui_setpoints");
		config_ptr->topics.agent_metrics = RosUtils::getParameter<std::string>(node, "agent_topics.metrics");
		config_ptr->topics.agent_markers = RosUtils::getParameter<std::string>(node, "agent_topics.markers");

		// Target topics
		config_ptr->topics.target_true_position = RosUtils::getParameter<std::string>(node, "target_topics.true_position");
		config_ptr->topics.target_est_position = RosUtils::getParameter<std::string>(node, "target_topics.est_position");
		config_ptr->topics.target_metrics = RosUtils::getParameter<std::string>(node, "target_topics.metrics");
		config_ptr->topics.target_markers = RosUtils::getParameter<std::string>(node, "target_topics.markers");

		// Cluster topics
		config_ptr->topics.cluster_assignment = RosUtils::getParameter<std::string>(node, "cluster_topics.assignment");
		config_ptr->topics.cluster_geometry = RosUtils::getParameter<std::string>(node, "cluster_topics.geometry");
		config_ptr->topics.cluster_metrics = RosUtils::getParameter<std::string>(node, "cluster_topics.metrics");
		config_ptr->topics.cluster_markers = RosUtils::getParameter<std::string>(node, "cluster_topics.markers");
	}

	void MissionSettingsParser::parseFrameParameters(const NodePtr& node, MissionConfigPtr& config_ptr)
	{
		// Global frames
		config_ptr->frames.world = RosUtils::getParameter<std::string>(node, "global_frames.world");

		// Agent frames
		config_ptr->frames.agent_local = RosUtils::getParameter<std::string>(node, "agent_frames.agent_local");
		config_ptr->frames.agent_body = RosUtils::getParameter<std::string>(node, "agent_frames.agent_body");
		config_ptr->frames.camera_body = RosUtils::getParameter<std::string>(node, "agent_frames.camera_body");
		config_ptr->frames.camera_optical = RosUtils::getParameter<std::string>(node, "agent_frames.camera_optical");
	}

} // namespace flychams::core