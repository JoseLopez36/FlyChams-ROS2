% ============================================================
% load_recording.m
%
% Load a FlyChams MCAP recording into a structured MATLAB struct.
%
% MCAP files are read with the MATLAB ROS 2 Toolbox (rosbag2) reader.
% The function discovers agents / targets / clusters automatically
% from the topic list, then deserialises every message type that the
% analysis scripts need.
%
% USAGE
%   rec = load_recording(mcap_path)
%   rec = load_recording(mcap_path, 'Verbose', true)
%
% INPUT
%   mcap_path  – absolute or relative path to the *.mcap file
%                (e.g. '../recordings/my_run/my_run_0.mcap')
%
% OUTPUT  rec  – struct with fields:
%
%   rec.meta
%       .recording_path   path supplied by the caller
%       .duration_s       total recording duration (s)
%       .n_agents         number of agents detected
%       .n_targets        number of targets detected
%       .n_clusters       number of clusters detected
%       .agent_ids        cell array of agent ID strings
%       .target_ids       cell array of target ID strings
%       .cluster_ids      cell array of cluster ID strings
%
%   rec.mission           MissionMetrics  struct-array (time-series)
%       .t, .total_agents, .total_targets, .total_clusters, .time
%
%   rec.fleet             FleetMetrics    struct-array
%       .t, .total_agents, .assignment_solve_duration
%
%   rec.agents.<AGENTID>
%       .t                Nx1  timestamps
%       .position         Nx3  [x y z] (m)  global position
%       .setpoint         Nx3  [x y z] (m)  global setpoint
%       .speed            Nx1  speed (m/s)
%       .distance_traveled Nx1  cumulative distance traveled (m)
%       .distance_to_goal Nx1  (m)
%       .pos_solve_ms     Nx1  position solve duration (ms)
%       .units            struct with one field per tracking unit (first unit ignored)
%           .<UNITID>
%               .zoom_factor   Nx1  zoom factor (focal for camera types (m), resolution factor for window types)
%               .image         Nx1  cell array of compressed image messages
%
%   rec.targets.<TARGETID>
%       .t                Nx1
%       .position         Nx3
%       .speed            Nx1
%       .distance_traveled Nx1
%
%   rec.clusters.<CLUSTERID>
%       .t                Nx1
%       .center           Nx3  [x y z] centroid
%       .radius           Nx1  enclosing circle radius (m)
%       .speed            Nx1
%       .distance_traveled Nx1
%
% REQUIREMENTS
%   MATLAB R2022b+  with  ROS Toolbox  (ros2bagreader)
%
% CUSTOM MESSAGE SETUP  (one-time, run once per machine)
%   flychams_api defines custom .msg types that MATLAB cannot deserialise
%   until stubs are generated.  From the matlab/ folder, run:
%
%     ros2genmsg('../src/')
%
%   Follow the printed instructions to add the generated folder to the
%   MATLAB path (savepath to make it permanent).  Re-run only when a
%   .msg field is added or renamed in flychams_api.
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-05-24
% ============================================================

function rec = load_recording(mcap_path, varargin)

    % --------------------------------------------------------
    % Parse options
    % --------------------------------------------------------
    p = inputParser;
    addRequired(p,  'mcap_path', @ischar);
    addParameter(p, 'Verbose',   false,  @islogical);
    parse(p, mcap_path, varargin{:});
    verbose = p.Results.Verbose;

    if verbose, fprintf('[load_recording] Opening: %s\n', mcap_path); end

    % --------------------------------------------------------
    % Open bag
    % --------------------------------------------------------
    bag = ros2bagreader(mcap_path);
    topic_list = bag.AvailableTopics;
    topic_names = topic_list.Row;
    t0 = bag.StartTime;
    t_end = bag.EndTime;

    % --------------------------------------------------------
    % Discover element IDs from topic names
    % --------------------------------------------------------
    agent_ids   = discover_ids(topic_names, '/flychams/agent/([^/]+)/global_position');
    target_ids  = discover_ids(topic_names, '/flychams/coordinator/([^/]+)/position');
    cluster_ids = discover_ids(topic_names, '/flychams/operator/([^/]+)/cluster_metrics');

    if verbose
        fprintf('[load_recording]  Agents  : %s\n', strjoin(agent_ids,   ', '));
        fprintf('[load_recording]  Targets : %s\n', strjoin(target_ids,  ', '));
        fprintf('[load_recording]  Clusters: %s\n', strjoin(cluster_ids, ', '));
    end

    % --------------------------------------------------------
    % Mission metrics  (operator node — may be absent in raw recordings)
    % --------------------------------------------------------
    rec.mission = struct('t', [], 'total_agents', [], 'total_targets', [], 'total_clusters', [], 'time', []);
    rec.mission = parse_mission_metrics(readMessages(select(bag, 'Topic', '/flychams/operator/mission_metrics')), t0);

    % --------------------------------------------------------
    % Fleet / assignment solve duration
    % Operator fleet_metrics (full) or raw coordinator Float32 topic
    % --------------------------------------------------------
    rec.fleet = struct('t', [], 'total_agents', [], 'assignment_solve_duration', []);
    rec.fleet = parse_fleet_metrics(readMessages(select(bag, 'Topic', '/flychams/operator/fleet_metrics')), t0);

    % --------------------------------------------------------
    % Per-agent data
    % --------------------------------------------------------
    rec.agents = struct();
    for i = 1 : numel(agent_ids)
        agent_id = agent_ids{i};
        if verbose, fprintf('[load_recording]  Agent: %s\n', agent_id); end
        rec.agents.(agent_id) = parse_agent(bag, agent_id, t0);
    end

    % --------------------------------------------------------
    % Per-target data
    % --------------------------------------------------------
    rec.targets = struct();
    for i = 1 : numel(target_ids)
        target_id = target_ids{i};
        if verbose, fprintf('[load_recording]  Target: %s\n', target_id); end
        rec.targets.(target_id) = parse_target(bag, target_id, t0);
    end

    % --------------------------------------------------------
    % Per-cluster data
    % --------------------------------------------------------
    rec.clusters = struct();
    for i = 1 : numel(cluster_ids)
        cluster_id = cluster_ids{i};
        if verbose, fprintf('[load_recording]  Cluster: %s\n', cluster_id); end
        rec.clusters.(cluster_id) = parse_cluster(bag, cluster_id, t0);
    end

    % --------------------------------------------------------
    % Metadata
    % --------------------------------------------------------
    rec.meta.recording_path = mcap_path;
    rec.meta.duration_s     = t_end - t0;
    rec.meta.n_agents        = numel(agent_ids);
    rec.meta.n_targets       = numel(target_ids);
    rec.meta.n_clusters      = numel(cluster_ids);
    rec.meta.agent_ids       = agent_ids;
    rec.meta.target_ids      = target_ids;
    rec.meta.cluster_ids     = cluster_ids;

    if verbose
        fprintf('[load_recording] Done. Duration = %.1f s\n', rec.meta.duration_s);
    end
end

% ============================================================
%  PARSERS
% ============================================================

function s = parse_mission_metrics(msgs, t0)
    N = numel(msgs);
    s.t              = zeros(N,1);
    s.total_agents   = zeros(N,1);
    s.total_targets  = zeros(N,1);
    s.total_clusters = zeros(N,1);
    s.time           = zeros(N,1);
    for k = 1:N
        m = msgs{k};
        s.t(k)              = stamp2sec(m.header.stamp) - t0;
        s.total_agents(k)   = m.total_agents;
        s.total_targets(k)  = m.total_targets;
        s.total_clusters(k) = m.total_clusters;
        s.time(k)           = m.time;
    end
end

function s = parse_fleet_metrics(msgs, t0)
    N = numel(msgs);
    s.t                        = zeros(N,1);
    s.total_agents             = zeros(N,1);
    s.assignment_solve_duration = zeros(N,1);
    for k = 1:N
        m = msgs{k};
        s.t(k)                         = stamp2sec(m.header.stamp) - t0;
        s.total_agents(k)              = m.total_agents;
        s.assignment_solve_duration(k) = m.assignment_solve_duration;
    end
end

function s = parse_agent(bag, agent_id, t0)
    s = struct('t',[], 'position',[], 'setpoint',[], ...
               'speed',[], 'distance_traveled',[], 'distance_to_goal',[], ...
               'pos_solve_ms',[], 'units', struct());

    % --- Observation setpoints ---
    msgs = readMessages(select(bag, 'Topic', ['/flychams/agent/' agent_id '/observation_setpoints']));
    N = numel(msgs);
    M = numel(msgs{end}.ids(2:end));
    unit_ids = msgs{end}.ids(2:end);

    % Pre-allocate zoom factor data for tracking units
    zoom_factor = cell(M, 1);
    for i = 1:M
        zoom_factor{i} = zeros(N, 1);
    end

    % Extract zoom factors for tracking units only (skip index 1 which is non-tracking)
    for k = 1:N
        m = msgs{k};
        for i = 1:M
            idx = i + 1;  % First unit is index 1 in zoom_factors, tracking units start at 2
            if idx <= numel(m.zoom_factors)
                zoom_factor{i}(k) = m.zoom_factors(idx);
            end
        end
    end

    % --- Agent metrics ---
    msgs = readMessages(select(bag, 'Topic', ['/flychams/operator/' agent_id '/agent_metrics']));
    N = numel(msgs);
    s.t                 = zeros(N,1);
    s.position          = zeros(N,3);
    s.setpoint          = zeros(N,3);
    s.speed             = zeros(N,1);
    s.distance_traveled = zeros(N,1);
    s.distance_to_goal  = zeros(N,1);
    s.pos_solve_ms      = zeros(N,1);

    for k = 1:N
        m = msgs{k};
        s.t(k)                  = stamp2sec(m.header.stamp) - t0;
        s.position(k,:)         = [m.position.x, m.position.y, m.position.z];
        s.setpoint(k,:)         = [m.setpoint.x, m.setpoint.y, m.setpoint.z];
        s.speed(k)              = m.speed;
        s.distance_traveled(k)  = m.distance_traveled;
        s.distance_to_goal(k)   = m.distance_to_goal;
        s.pos_solve_ms(k)       = m.position_solve_duration;
    end

    % --- Per-unit data ---
    for i = 1:M
        unit_id = unit_ids{i};
        s.units.(unit_id) = struct('zoom_factor', zoom_factor{i});
    end

    % --- Compressed images ---
    for i = 1:M
        unit_id = unit_ids{i};
        msgs = readMessages(select(bag, 'Topic', ['/flychams/agent/' agent_id '/' unit_id '/image/compressed']));
        s.units.(unit_id).image = msgs;
    end
end

function s = parse_target(bag, target_id, t0)
    s = struct('t',[], 'position',[], 'speed',[], 'distance_traveled',[]);

    % Target metrics
    msgs = readMessages(select(bag, 'Topic', ['/flychams/operator/' target_id '/target_metrics']));
    N = numel(msgs);
    s.t                 = zeros(N,1);
    s.position          = zeros(N,3);
    s.speed             = zeros(N,1);
    s.distance_traveled = zeros(N,1);
    for k = 1:N
        m = msgs{k};
        s.t(k)                  = stamp2sec(m.header.stamp) - t0;
        s.position(k,:)         = [m.position.x, m.position.y, m.position.z];
        s.speed(k)              = m.speed;
        s.distance_traveled(k)  = m.distance_traveled;
    end
end

function s = parse_cluster(bag, cluster_id, t0)
    s = struct('t',[], 'center',[], 'radius',[], 'speed',[], ...
               'distance_traveled',[]);

    % Cluster metrics
    msgs = readMessages(select(bag, 'Topic', ['/flychams/operator/' cluster_id '/cluster_metrics']));
    N = numel(msgs);
    s.t                  = zeros(N,1);
    s.center             = zeros(N,3);
    s.radius             = zeros(N,1);
    s.speed              = zeros(N,1);
    s.distance_traveled  = zeros(N,1);
    for k = 1:N
        m = msgs{k};
        s.t(k)                 = stamp2sec(m.header.stamp) - t0;
        s.center(k,:)          = [m.center.x, m.center.y, m.center.z];
        s.radius(k)            = m.radius;
        s.speed(k)             = m.speed;
        s.distance_traveled(k) = m.distance_traveled;
    end
end

% ============================================================
%  UTILITIES
% ============================================================

function ids = discover_ids(topic_names, pattern)
    ids = {};
    for i = 1:numel(topic_names)
        tok = regexp(topic_names{i}, pattern, 'tokens', 'once');
        if ~isempty(tok)
            id = tok{1};
            if ~ismember(id, ids)
                ids{end+1} = id;
            end
        end
    end
    ids = sort(ids);
end

function s = stamp2sec(stamp)
    % Convert ROS2 builtin_interfaces/Time to seconds.
    s = double(stamp.sec) + double(stamp.nanosec) * 1e-9;
end