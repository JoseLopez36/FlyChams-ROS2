% ============================================================
% analyze_tracking.m
%
% Tracking-unit metrics analysis from FlyChams MCAP bags.
%
% Sources (per bag):
%   /flychams/operator/AGENTID/agent_metrics  (AgentMetrics)
%
% USAGE
%   analyze_tracking(bag_path)
%   analyze_tracking(bag_path, t_start, t_end)   % crop to [t_start, t_end] seconds
%
% EXAMPLE
%   analyze_tracking('../recordings/MyRun/MyRun_0.mcap', 20.0, 110.0)
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-06-07
% ============================================================

function analyze_tracking(bag_path, t_start, t_end)

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
        topic = metrics_topics{i};
        agent_id = extract_agent_id(topic, '/flychams/operator/', '/agent_metrics');

        sel = select(bag, 'Topic', topic);
        msgs = readMessages(sel);
        t_raw = bag_timestamps(sel);

        max_units = max(cellfun(@(m) max(numel(m.upsilons_norm), numel(m.apparent_target_sizes)), msgs)) - 1;
        ups = nan(numel(msgs), max_units);
        siz = nan(numel(msgs), max_units);

        for k = 1:numel(msgs)
            ups_k = double(msgs{k}.upsilons_norm);
            siz_k = double(msgs{k}.apparent_target_sizes);
            ups(k, 1:numel(ups_k)-1) = ups_k(2:end);
            siz(k, 1:numel(siz_k)-1) = siz_k(2:end);
        end

        [ups, t] = crop_matrix(ups, t_raw, t_start, t_end);
        [siz, ~] = crop_matrix(siz, t_raw, t_start, t_end);

        data.agents(i).id = agent_id;
        data.agents(i).t = t;
        data.agents(i).upsilons_norm = ups;
        data.agents(i).apparent_target_sizes = siz;
    end
end

% ============================================================
%  PLOT
% ============================================================

function plot_bag(data, label)
    palette = agent_palette();

    fig = figure('Name', sprintf('Tracking Upsilon - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [100, 100, 1100, 650]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Normalized Zoom Factor', 'FontSize',12);
    xlabel(ax, 'Time (s)', 'FontSize',10);
    ylabel(ax, 'Upsilon (-)', 'FontSize',10);
    plot_units(ax, data, 'upsilons_norm', palette);
    ylim(ax, [0 1]);
    sgtitle(fig, sprintf('Normalized Zoom Factor  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('Tracking Apparent Size - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [160, 120, 1100, 650]);

    ax = paper_ax(fig, 1, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Apparent Target Size', 'FontSize',12);
    xlabel(ax, 'Time (s)', 'FontSize',10);
    ylabel(ax, 'Radius (px)', 'FontSize',10);
    plot_units(ax, data, 'apparent_target_sizes', palette);
    sgtitle(fig, sprintf('Apparent Target Size  |  %s', label), 'FontSize',13);

    fig = figure('Name', sprintf('Tracking Statistics - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [280, 160, 900, 900]);

    ax = paper_ax(fig, 1, 1, 1);
    axis(ax, 'off');
    title(ax, 'Summary Statistics', 'FontSize',12);
    txt = summary_lines(data);
    text(ax, 0.03, 0.98, txt, 'Units','normalized','VerticalAlignment','top', ...
         'Color',[0.10 0.10 0.10], 'FontSize',10, 'FontName','Monospaced');
    sgtitle(fig, sprintf('Tracking Statistics  |  %s', label), 'FontSize',13);

    fprintf('\n=== %s ===\n', label);
    fprintf('%s\n', strjoin(txt, newline));
end

% ============================================================
%  HELPERS
% ============================================================

function plot_units(ax, data, field, palette)
    for a = 1:numel(data.agents)
        col = palette(1 + mod(a-1, size(palette,1)), :);
        vals = data.agents(a).(field);
        for u = 1:size(vals, 2)
            plot(ax, data.agents(a).t, vals(:,u), '-', 'Color', col, 'LineWidth', 1.5, ...
                'DisplayName', sprintf('%s u%d', data.agents(a).id, u));
        end
    end
    legend(ax, 'Location','best', 'FontSize',7, 'Box','off');
end

function txt = summary_lines(data)
    all_ups = [];
    all_siz = [];

    for a = 1:numel(data.agents)
        ups = data.agents(a).upsilons_norm(:);
        siz = data.agents(a).apparent_target_sizes(:);
        ups = ups(~isnan(ups));
        siz = siz(~isnan(siz));
        all_ups = [all_ups; ups];
        all_siz = [all_siz; siz];
    end

    txt = {
        '--- Fleet ---'
        sprintf('Agents              : %d', numel(data.agents))
        sprintf('Units per agent     : %d', size(data.agents(1).upsilons_norm, 2))
        sprintf('Upsilon mean        : %.3f', mean(all_ups))
        sprintf('Upsilon median      : %.3f', median(all_ups))
        sprintf('Upsilon p95         : %.3f', prctile(all_ups,95))
        sprintf('Size mean           : %.3f px', mean(all_siz))
        sprintf('Size median         : %.3f px', median(all_siz))
        sprintf('Size p95            : %.3f px', prctile(all_siz,95))
        ''
    };

    for a = 1:numel(data.agents)
        txt{end+1,1} = sprintf('--- Agent %s ---', data.agents(a).id);
        for u = 1:size(data.agents(a).upsilons_norm, 2)
            ups = data.agents(a).upsilons_norm(:,u);
            siz = data.agents(a).apparent_target_sizes(:,u);
            ups = ups(~isnan(ups));
            siz = siz(~isnan(siz));
            txt{end+1,1} = sprintf('Unit %d upsilon: mean %.3f  p95 %.3f', u, mean(ups), prctile(ups,95));
            txt{end+1,1} = sprintf('Unit %d size   : mean %.3f  p95 %.3f px', u, mean(siz), prctile(siz,95));
        end
        txt{end+1,1} = '';
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

function [vals, t] = crop_matrix(vals, t_raw, t_start, t_end)
    mask = t_raw >= t_start & t_raw <= t_end;
    vals = vals(mask,:);
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