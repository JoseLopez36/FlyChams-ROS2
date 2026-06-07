% ============================================================
% analyze_trajectories.m
%
% Trajectory analysis from FlyChams MCAP bags.
%
% Sources (per bag):
%   /flychams/operator/AGENTID/agent_metrics         (AgentMetrics)
%   /flychams/coordinator/AGENTID/clusters           (AgentClusters)
%
% USAGE
%   analyze_trajectories(bag_path)
%   analyze_trajectories(bag_path, t_start, t_end)   % crop to [t_start, t_end] seconds
%
% EXAMPLE
%   analyze_trajectories('../recordings/MyRun/MyRun_0.mcap', 20.0, 110.0)
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-06-07
% ============================================================

function analyze_trajectories(bag_path, t_start, t_end)

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

    topics = bag_topics(bag);
    metrics_topics = topics(contains(topics, '/flychams/operator/') & endsWith(topics, '/agent_metrics'));

    data.agents = struct([]);
    for i = 1:numel(metrics_topics)
        metrics_topic = metrics_topics{i};
        agent_id = extract_agent_id(metrics_topic, '/flychams/operator/', '/agent_metrics');
        clusters_topic = ['/flychams/coordinator/' agent_id '/clusters'];

        met_sel = select(bag, 'Topic', metrics_topic);
        met_msgs = readMessages(met_sel);
        t_met_raw = bag_timestamps(met_sel);

        pos = zeros(numel(met_msgs), 3);
        setp = zeros(numel(met_msgs), 3);
        dist = zeros(numel(met_msgs), 1);
        speed = zeros(numel(met_msgs), 1);
        goal = zeros(numel(met_msgs), 1);
        for k = 1:numel(met_msgs)
            pos(k,:) = point_xyz(met_msgs{k}.position);
            setp(k,:) = point_xyz(met_msgs{k}.setpoint);
            dist(k) = double(met_msgs{k}.distance_traveled);
            speed(k) = double(met_msgs{k}.speed);
            goal(k) = double(met_msgs{k}.distance_to_goal);
        end

        [pos, t_met] = crop_matrix(pos, t_met_raw, t_start, t_end);
        [setp, ~] = crop_matrix(setp, t_met_raw, t_start, t_end);
        [dist, ~] = crop_vector(dist, t_met_raw, t_start, t_end);
        [speed, ~] = crop_vector(speed, t_met_raw, t_start, t_end);
        [goal, ~] = crop_vector(goal, t_met_raw, t_start, t_end);

        clu_sel = select(bag, 'Topic', clusters_topic);
        clu_msgs = readMessages(clu_sel);
        t_clu_raw = bag_timestamps(clu_sel);
        max_units = max(cellfun(@(m) numel(m.centers), clu_msgs));
        centers = nan(numel(clu_msgs), max_units, 3);
        radii = nan(numel(clu_msgs), max_units);
        for k = 1:numel(clu_msgs)
            for u = 1:numel(clu_msgs{k}.centers)
                centers(k,u,:) = point_xyz(clu_msgs{k}.centers(u));
                radii(k,u) = double(clu_msgs{k}.radii(u));
            end
        end
        [centers, t_clu] = crop_centers(centers, t_clu_raw, t_start, t_end);
        [radii, ~] = crop_matrix(radii, t_clu_raw, t_start, t_end);

        data.agents(i).id = agent_id;
        data.agents(i).t_agent = t_met;
        data.agents(i).position = pos;
        data.agents(i).setpoint = setp;
        data.agents(i).distance_traveled = dist;
        data.agents(i).speed = speed;
        data.agents(i).distance_to_goal = goal;
        data.agents(i).t_clusters = t_clu;
        data.agents(i).centers = centers;
        data.agents(i).radii = radii;
    end
end

% ============================================================
%  PLOT
% ============================================================

function plot_bag(data, label)
    palette = agent_palette();

    fig = figure('Name', sprintf('Top-Down Trajectories - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [100, 100, 1100, 900]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on'); axis(ax,'equal');
    title(ax, 'Top-Down Trajectories', 'FontSize',12);
    xlabel(ax, 'X (m)', 'FontSize',10);
    ylabel(ax, 'Y (m)', 'FontSize',10);
    plot_xy(ax, data, palette);
    sgtitle(fig, sprintf('Top-Down Trajectories  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('3-D Trajectories - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [160, 120, 1100, 900]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on'); view(ax, 3);
    title(ax, '3-D Trajectories', 'FontSize',12);
    xlabel(ax, 'X (m)', 'FontSize',10);
    ylabel(ax, 'Y (m)', 'FontSize',10);
    zlabel(ax, 'Z (m)', 'FontSize',10);
    plot_xyz(ax, data, palette);
    sgtitle(fig, sprintf('3-D Trajectories  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('Agent Movement - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [220, 140, 1100, 650]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Agent Movement', 'FontSize',12);
    xlabel(ax, 'Time (s)', 'FontSize',10);
    ylabel(ax, 'Distance / Speed', 'FontSize',10);
    plot_motion(ax, data, palette);
    sgtitle(fig, sprintf('Agent Movement  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('Trajectory Statistics - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [280, 160, 900, 900]);

    ax = paper_ax(fig, 1, 1, 1);
    axis(ax, 'off');
    title(ax, 'Summary Statistics', 'FontSize',12);
    txt = summary_lines(data);
    text(ax, 0.03, 0.98, txt, 'Units','normalized','VerticalAlignment','top', ...
         'Color',[0.10 0.10 0.10], 'FontSize',10, 'FontName','Monospaced');
    sgtitle(fig, sprintf('Trajectory Statistics  |  %s', label), 'FontSize',13);

    fprintf('\n=== %s ===\n', label);
    fprintf('%s\n', strjoin(txt, newline));
end

% ============================================================
%  HELPERS
% ============================================================

function plot_xy(ax, data, palette)
    for a = 1:numel(data.agents)
        col = palette(1 + mod(a-1, size(palette,1)), :);
        p = data.agents(a).position;
        plot(ax, p(:,1), p(:,2), '-', 'Color', col, 'LineWidth', 2.4, ...
            'DisplayName', sprintf('%s agent', data.agents(a).id));
        plot(ax, p(1,1), p(1,2), 'o', 'Color', col, 'MarkerFaceColor', [1 1 1], ...
            'MarkerSize', 7, 'LineWidth', 1.3, 'HandleVisibility','off');
        plot(ax, p(end,1), p(end,2), 'x', 'Color', col, ...
            'MarkerSize', 8, 'LineWidth', 1.5, 'HandleVisibility','off');

        c = data.agents(a).centers;
        light_col = lighten_color(col, 0.55);
        for u = 1:size(c, 2)
            p = squeeze(c(:,u,:));
            plot(ax, p(:,1), p(:,2), '-', 'Color', light_col, 'LineWidth', 1.6, ...
                'DisplayName', sprintf('%s cluster u%d', data.agents(a).id, u));
            plot(ax, p(1,1), p(1,2), 'o', 'Color', light_col, 'MarkerFaceColor', [1 1 1], ...
                'MarkerSize', 7, 'LineWidth', 1.0, 'HandleVisibility','off');
            plot(ax, p(end,1), p(end,2), 'x', 'Color', light_col, ...
                'MarkerSize', 8, 'LineWidth', 1.2, 'HandleVisibility','off');
        end
    end
    legend(ax, 'Location','bestoutside', 'FontSize',7, 'Box','off');
end

function plot_xyz(ax, data, palette)
    for a = 1:numel(data.agents)
        col = palette(1 + mod(a-1, size(palette,1)), :);
        p = data.agents(a).position;
        plot3(ax, p(:,1), p(:,2), p(:,3), '-', 'Color', col, 'LineWidth', 1.6);
        c = data.agents(a).centers;
        for u = 1:size(c, 2)
            plot3(ax, c(:,u,1), c(:,u,2), c(:,u,3), '--', 'Color', col, 'LineWidth', 1.0);
        end
    end
end

function plot_motion(ax, data, palette)
    yyaxis(ax, 'left');
    ylabel(ax, 'Travel distance (m)', 'FontSize',9);
    for a = 1:numel(data.agents)
        col = palette(1 + mod(a-1, size(palette,1)), :);
        plot(ax, data.agents(a).t_agent, data.agents(a).distance_traveled, '-', ...
            'Color', col, 'LineWidth', 1.3, 'DisplayName', [data.agents(a).id ' distance']);
    end
    yyaxis(ax, 'right');
    ylabel(ax, 'Speed (m/s)', 'FontSize',9);
    for a = 1:numel(data.agents)
        col = palette(1 + mod(a-1, size(palette,1)), :);
        plot(ax, data.agents(a).t_agent, data.agents(a).speed, ':', ...
            'Color', col, 'LineWidth', 1.1, 'DisplayName', [data.agents(a).id ' speed']);
    end
    legend(ax, 'Location','best', 'FontSize',7, 'Box','off');
end

function txt = summary_lines(data)
    txt = {};
    fleet_distance = 0;
    for a = 1:numel(data.agents)
        d = data.agents(a).distance_traveled;
        sp = data.agents(a).speed;
        goal = data.agents(a).distance_to_goal;
        fleet_distance = fleet_distance + d(end);
        txt{end+1,1} = sprintf('--- Agent %s ---', data.agents(a).id);
        txt{end+1,1} = sprintf('Travel distance : %.3f m', d(end));
        txt{end+1,1} = sprintf('Mean speed      : %.3f m/s', mean(sp));
        txt{end+1,1} = sprintf('Max speed       : %.3f m/s', max(sp));
        txt{end+1,1} = sprintf('Mean goal dist  : %.3f m', mean(goal));
        for u = 1:size(data.agents(a).centers, 2)
            c = squeeze(data.agents(a).centers(:,u,:));
            txt{end+1,1} = sprintf('Cluster u%d path : %.3f m', u, path_length(c));
        end
        txt{end+1,1} = '';
    end
    txt = [{'--- Fleet ---'}; {sprintf('Total agent travel: %.3f m', fleet_distance)}; {''}; txt];
end

function d = path_length(points)
    dp = diff(points, 1, 1);
    d = sum(sqrt(sum(dp.^2, 2)), 'omitnan');
end

function xyz = point_xyz(p)
    xyz = [double(p.x), double(p.y), double(p.z)];
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

function [vals, t] = crop_matrix(vals, t_raw, t_start, t_end)
    mask = t_raw >= t_start & t_raw <= t_end;
    vals = vals(mask,:);
    t = t_raw(mask) - t_raw(find(mask,1));
end

function [vals, t] = crop_centers(vals, t_raw, t_start, t_end)
    mask = t_raw >= t_start & t_raw <= t_end;
    vals = vals(mask,:,:);
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

function col = lighten_color(col, amount)
    col = col + amount * (1.0 - col);
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