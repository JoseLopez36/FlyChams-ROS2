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
    [~, stem] = fileparts(bag_path);
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
    palette = agent_palette();

    fig = figure('Name', sprintf('Fleet Assignment - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [100, 100, 1100, 650]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Fleet Assignment', 'FontSize',12);
    xlabel(ax, 'Time (s)', 'FontSize',10);
    yyaxis(ax, 'left');
    ylabel(ax, 'Swap count', 'FontSize',10);
    plot(ax, data.t_fleet, data.assignment_swap_count, '-', 'Color', [0.85 0.33 0.10], 'LineWidth', 1.8);
    yyaxis(ax, 'right');
    ylabel(ax, 'Solve duration (ms)', 'FontSize',10);
    plot(ax, data.t_fleet, data.assignment_solve_duration, '-', 'Color', [0.00 0.45 0.74], 'LineWidth', 1.6);
    sgtitle(fig, sprintf('Fleet Assignment  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('Agent Travel Distance - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [160, 120, 1100, 650]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Agent Travel Distance', 'FontSize',12);
    xlabel(ax, 'Time (s)', 'FontSize',10);
    ylabel(ax, 'Distance (m)', 'FontSize',10);
    for a = 1:numel(data.agents)
        col = palette(1 + mod(a-1, size(palette,1)), :);
        plot(ax, data.agents(a).t, data.agents(a).distance_traveled, '-', ...
            'Color', col, 'LineWidth', 1.8, 'DisplayName', data.agents(a).id);
    end
    legend(ax, 'Location','best', 'FontSize',7, 'Box','off');
    sgtitle(fig, sprintf('Agent Travel Distance  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('Fleet Motion and Goal Error - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [220, 140, 1100, 650]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Fleet Motion and Goal Error', 'FontSize',12);
    xlabel(ax, 'Time (s)', 'FontSize',10);
    yyaxis(ax, 'left');
    ylabel(ax, 'Mean speed (m/s)', 'FontSize',10);
    [t, mean_speed] = mean_agent_series(data.agents, 'speed');
    plot(ax, t, mean_speed, '-', 'Color', [0.20 0.80 0.40], 'LineWidth', 1.8);
    yyaxis(ax, 'right');
    ylabel(ax, 'Mean distance to goal (m)', 'FontSize',10);
    [t, mean_goal] = mean_agent_series(data.agents, 'distance_to_goal');
    plot(ax, t, mean_goal, '-', 'Color', [0.55 0.20 1.00], 'LineWidth', 1.8);
    sgtitle(fig, sprintf('Fleet Motion and Goal Error  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('Fleet Statistics - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [280, 160, 900, 900]);

    ax = paper_ax(fig, 1, 1, 1);
    axis(ax, 'off');
    title(ax, 'Summary Statistics', 'FontSize',12);
    txt = summary_lines(data);
    text(ax, 0.03, 0.98, txt, 'Units','normalized','VerticalAlignment','top', ...
         'Color',[0.10 0.10 0.10], 'FontSize',10, 'FontName','Monospaced');
    sgtitle(fig, sprintf('Fleet Statistics  |  %s', label), 'FontSize',13);

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

function colors = agent_palette()
    colors = [
        0.00 0.85 1.00
        0.20 0.80 0.40
        1.00 0.78 0.00
        0.55 0.20 1.00
        1.00 0.65 0.40
        0.00 0.55 1.00
        0.00 0.80 0.65
        0.85 0.00 0.85
    ];
end

function ax = paper_ax(fig, rows, cols, idx)
    ax = subplot(rows, cols, idx, 'Parent', fig);
    set(ax, 'Color',     [1.0  1.0  1.0 ], ...
            'XColor',    [0.10 0.10 0.10], ...
            'YColor',    [0.10 0.10 0.10], ...
            'GridColor', [0.75 0.75 0.75], ...
            'GridAlpha', 0.45, ...
            'LineWidth', 0.8, ...
            'FontSize',  8);
end