% ============================================================
% analyze_fleet.m
%
% Fleet-level metrics analysis from FlyChams MCAP bags.
%
% Sources (per bag):
%   /flychams/operator/fleet_metrics                    (FleetMetrics)
%   /flychams/operator/AGENTID/agent_metrics            (AgentMetrics)
%
% USAGE
%   analyze_fleet(bag_path)
%   analyze_fleet(bag_path, t_start, t_end)   % crop to [t_start, t_end] seconds
%
% EXAMPLE
%   analyze_fleet('../recordings/MyRun/MyRun_0.mcap', 20.0, 110.0)
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-06-07
% ============================================================

function analyze_fleet(bag_path, t_start, t_end)

    if nargin < 1 || isempty(bag_path)
        [fname, fpath] = uigetfile({'*.mcap','MCAP files (*.mcap)'});
        if isequal(fname, 0), return; end
        bag_path = fullfile(fpath, fname);
    end

    if nargin < 2, t_start = -inf; end
    if nargin < 3, t_end   =  inf; end

    data = load_bag(bag_path, t_start, t_end);
    stem = analysis_common('bag_label', bag_path);
    plot_bag(data, stem);
end

% ============================================================
%  LOAD
% ============================================================

function data = load_bag(bag_path, t_start, t_end)
    bag = ros2bagreader(bag_path);

    fleet_topic = '/flychams/operator/fleet_metrics';
    fleet_sel = select(bag, 'Topic', fleet_topic);
    fleet_msgs = readMessages(fleet_sel);
    t_fleet_raw = bag_timestamps(fleet_sel);

    data.total_agents = cellfun(@(m) double(m.total_agents), fleet_msgs);
    data.assignment_swap_count = cellfun(@(m) double(m.assignment_swap_count), fleet_msgs);
    data.assignment_solve_duration = cellfun(@(m) double(m.assignment_solve_duration), fleet_msgs);

    [data.total_agents, data.t_fleet] = crop_vector(data.total_agents, t_fleet_raw, t_start, t_end);
    [data.assignment_swap_count, ~] = crop_vector(data.assignment_swap_count, t_fleet_raw, t_start, t_end);
    [data.assignment_solve_duration, ~] = crop_vector(data.assignment_solve_duration, t_fleet_raw, t_start, t_end);

    topics = bag_topics(bag);
    metrics_topics = topics(contains(topics, '/flychams/operator/') & endsWith(topics, '/agent_metrics'));

    data.agents = struct([]);
    for i = 1:numel(metrics_topics)
        topic = metrics_topics{i};
        agent_id = extract_agent_id(topic, '/flychams/operator/', '/agent_metrics');

        sel = select(bag, 'Topic', topic);
        msgs = readMessages(sel);
        t_raw = bag_timestamps(sel);

        distance = cellfun(@(m) double(m.distance_traveled), msgs);
        speed = cellfun(@(m) double(m.speed), msgs);
        goal = cellfun(@(m) double(m.distance_to_goal), msgs);
        solve = cellfun(@(m) double(m.position_solve_duration), msgs);

        [distance, t] = crop_vector(distance, t_raw, t_start, t_end);
        [speed, ~] = crop_vector(speed, t_raw, t_start, t_end);
        [goal, ~] = crop_vector(goal, t_raw, t_start, t_end);
        [solve, ~] = crop_vector(solve, t_raw, t_start, t_end);

        data.agents(i).id = agent_id;
        data.agents(i).t = t;
        data.agents(i).distance_traveled = distance;
        data.agents(i).speed = speed;
        data.agents(i).distance_to_goal = goal;
        data.agents(i).position_solve_duration = solve;
    end

    data.fleet_distance = sum(arrayfun(@(a) a.distance_traveled(end), data.agents));
end

% ============================================================
%  PLOT
% ============================================================

function plot_bag(data, label)
    palette = analysis_common('palette');
    style = analysis_common('style');
    out_fig_dir = analysis_common('figures_dir');
    out_stats_dir = analysis_common('stats_dir');

    fig = figure('Name', sprintf('Fleet Assignment - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Units', 'inches', 'Position', [0, 0, style.double_width, style.short_height]);

    ax = analysis_common('axis', fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    xlabel(ax, '$time\,[s]$', 'Interpreter','latex');
    yyaxis(ax, 'left');
    ylabel(ax, '$N_{swaps}$', 'Interpreter','latex', 'Color', style.orange);
    plot(ax, data.t_fleet, data.assignment_swap_count, '-', 'Color', style.orange, 'LineWidth', style.line_width);
    yyaxis(ax, 'right');
    ylabel(ax, '$t_{solve}\,[ms]$', 'Interpreter','latex', 'Color', style.blue);
    plot(ax, data.t_fleet, data.assignment_solve_duration, '-', 'Color', style.blue, 'LineWidth', style.line_width);
    analysis_common('export_figure', fig, out_fig_dir, label, 'fleet_assignment');

    fig = figure('Name', sprintf('Agent Travel Distance - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Units', 'inches', 'Position', [0, 0, style.double_width, style.short_height]);

    ax = analysis_common('axis', fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    xlabel(ax, '$time\,[s]$', 'Interpreter','latex');
    ylabel(ax, '$d\,[m]$', 'Interpreter','latex');
    for a = 1:numel(data.agents)
        col = palette(1 + mod(a-1, size(palette,1)), :);
        plot(ax, data.agents(a).t, data.agents(a).distance_traveled, '-', ...
            'Color', col, 'LineWidth', style.line_width, 'DisplayName', data.agents(a).id);
    end
    analysis_common('export_figure', fig, out_fig_dir, label, 'agent_travel_distance');

    fig = figure('Name', sprintf('Fleet Motion and Goal Error - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Units', 'inches', 'Position', [0, 0, style.double_width, style.short_height]);

    ax = analysis_common('axis', fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    xlabel(ax, '$time\,[s]$', 'Interpreter','latex');
    yyaxis(ax, 'left');
    ylabel(ax, '$\bar{v}\,[m/s]$', 'Interpreter','latex', 'Color', style.green);
    [t, mean_speed] = mean_agent_series(data.agents, 'speed');
    plot(ax, t, mean_speed, '-', 'Color', style.green, 'LineWidth', style.line_width);
    yyaxis(ax, 'right');
    ylabel(ax, '$\bar{e}_{goal}\,[m]$', 'Interpreter','latex', 'Color', style.purple);
    [t, mean_goal] = mean_agent_series(data.agents, 'distance_to_goal');
    plot(ax, t, mean_goal, '-', 'Color', style.purple, 'LineWidth', style.line_width);
    analysis_common('export_figure', fig, out_fig_dir, label, 'fleet_motion_goal_error');

    txt = summary_lines(data);
    analysis_common('export_stats', out_stats_dir, label, 'fleet', txt);

    fprintf('\n=== %s ===\n', label);
    fprintf('%s\n', strjoin(txt, newline));
end

% ============================================================
%  HELPERS
% ============================================================

function [t, avg] = mean_agent_series(agents, field)
    t = agents(1).t(:);
    values = nan(numel(t), numel(agents));
    for a = 1:numel(agents)
        series = agents(a).(field);
        values(:,a) = interp1(agents(a).t(:), series(:), t, 'linear', 'extrap');
    end
    avg = mean(values, 2, 'omitnan');
end

function txt = summary_lines(data)
    agent_travel = arrayfun(@(a) a.distance_traveled(end), data.agents);
    agent_speed = arrayfun(@(a) mean(a.speed), data.agents);
    agent_goal = arrayfun(@(a) mean(a.distance_to_goal), data.agents);
    position_solve = [];
    for a = 1:numel(data.agents)
        position_solve = [position_solve; data.agents(a).position_solve_duration(:)];
    end

    txt = {
        '--- Fleet ---'
        sprintf('Agents              : %.0f', max(data.total_agents))
        sprintf('Total fleet travel        : %.3f m', data.fleet_distance)
        sprintf('Mean agent travel   : %.3f m', mean(agent_travel))
        sprintf('Std agent travel    : %.3f m', std(agent_travel))
        sprintf('Max agent travel    : %.3f m', max(agent_travel))
        sprintf('Mean agent speed    : %.3f m/s', mean(agent_speed))
        sprintf('Mean goal distance  : %.3f m', mean(agent_goal))
        sprintf('Assignment swaps    : %.0f', data.assignment_swap_count(end))
        sprintf('Swap rate           : %.3f swaps/min', 60.0 * data.assignment_swap_count(end) / data.t_fleet(end))
        sprintf('Assign solve mean   : %.3f ms', mean(data.assignment_solve_duration))
        sprintf('Assign solve p95    : %.3f ms', prctile(data.assignment_solve_duration,95))
        sprintf('Pos solve mean      : %.3f ms', mean(position_solve))
        sprintf('Pos solve p95       : %.3f ms', prctile(position_solve,95))
        ''
        '--- Agents ---'
    };

    for a = 1:numel(data.agents)
        txt{end+1,1} = sprintf('%s travel      : %.3f m', data.agents(a).id, data.agents(a).distance_traveled(end));
        txt{end+1,1} = sprintf('%s mean speed  : %.3f m/s', data.agents(a).id, mean(data.agents(a).speed));
        txt{end+1,1} = sprintf('%s goal mean   : %.3f m', data.agents(a).id, mean(data.agents(a).distance_to_goal));
        txt{end+1,1} = sprintf('%s pos p95     : %.3f ms', data.agents(a).id, prctile(data.agents(a).position_solve_duration,95));
    end
end

function topics = bag_topics(bag)
    topics = bag.AvailableTopics.Properties.RowNames;
end

function id = extract_agent_id(topic, prefix, suffix)
    id = erase(erase(topic, prefix), suffix);
end

function t = bag_timestamps(sel)
    tlist = sel.MessageList.Time;
    t = seconds(seconds(tlist - tlist(1)));
end

function [vals, t] = crop_vector(vals, t_raw, t_start, t_end)
    mask = t_raw >= t_start & t_raw <= t_end;
    vals = vals(mask);
    t = t_raw(mask) - t_raw(find(mask,1));
end