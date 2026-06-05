% ============================================================
% analyze_assignment_solvers.m
%
% Assignment solver benchmark analysis from benchmark MCAP bags.
%
% Sources (per bag):
%   /flychams/coordinator/assignment_solve_duration  (std_msgs/Float32, ms)
%   /flychams/coordinator/assignment_node_count      (std_msgs/Int32)
%
% USAGE
%   analyze_assignment_solvers(bag_path)
%   analyze_assignment_solvers(bag_path, t_start, t_end)   % crop to [t_start, t_end] seconds
%
% EXAMPLE
%   analyze_assignment_solvers('../recordings/Assignment-Benchmark-1-Result-Exhaustive/Assignment-Benchmark-1-Result-Exhaustive_0.mcap', 20.0, 110.0)
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-06-05
% ============================================================

function analyze_assignment_solvers(bag_path, t_start, t_end)

    if nargin < 1 || isempty(bag_path)
        [fname, fpath] = uigetfile({'*.mcap','MCAP files (*.mcap)'});
        if isequal(fname, 0), return; end
        if ischar(fname)
            bag_path = fullfile(fpath, fname);
        else
            bag_path = cellfun(@(f) fullfile(fpath, f), fname, 'UniformOutput', false);
        end
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

    dur_topic = '/flychams/coordinator/assignment_solve_duration';
    cnt_topic = '/flychams/coordinator/assignment_node_count';

    % Solve duration
    dur_sel          = select(bag, 'Topic', dur_topic);
    dur_msgs         = readMessages(dur_sel);
    data.duration_ms = cellfun(@(m) double(m.data), dur_msgs);
    t_dur_raw        = bag_timestamps(dur_sel);

    % Node count
    cnt_sel         = select(bag, 'Topic', cnt_topic);
    cnt_msgs        = readMessages(cnt_sel);
    data.node_count = cellfun(@(m) double(m.data), cnt_msgs);
    t_cnt_raw       = bag_timestamps(cnt_sel);

    % Crop to [t_start, t_end] and re-zero
    [data.duration_ms, data.t_duration] = crop(data.duration_ms, t_dur_raw, t_start, t_end);
    [data.node_count,  data.t_count   ] = crop(data.node_count,  t_cnt_raw, t_start, t_end);
end

% ============================================================
%  PLOT
% ============================================================

function plot_bag(data, label)
    col_dur = [0.85 0.33 0.10];
    col_cnt = [0.00 0.45 0.74];

    fig = figure('Name', sprintf('Assignment Solver — %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Position', [100, 100, 1600, 500]);

    % ---- Solve duration ----
    ms = data.duration_ms(:);
    t  = data.t_duration(:);

    ax = light_ax(fig, 1, 3, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Solve Duration (ms)', 'FontSize',9);
    xlabel(ax, 'Time (s)', 'FontSize',8);
    plot(ax, t, ms, '-', 'Color', col_dur, 'LineWidth', 1.2);
    yline(ax, mean(ms),       '--', 'Color', col_dur,      'LineWidth', 1.0);
    yline(ax, prctile(ms,99), ':',  'Color', [0.8 0.1 0.1],'LineWidth', 1.0);

    ax = light_ax(fig, 1, 3, 2);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Node Count', 'FontSize',9);
    xlabel(ax, 'Time (s)', 'FontSize',8);
    nc = data.node_count(:);
    tc = data.t_count(:);
    plot(ax, tc, nc, '-', 'Color', col_cnt, 'LineWidth', 1.2);
    yline(ax, mean(nc),       '--', 'Color', col_cnt,       'LineWidth', 1.0);
    yline(ax, prctile(nc,99), ':',  'Color', [0.8 0.1 0.1], 'LineWidth', 1.0);

    % ---- Summary ----
    ax = light_ax(fig, 1, 3, 3);
    axis(ax, 'off');
    title(ax, 'Summary Statistics', 'FontSize',9);
    txt = [
        {'--- Solve Duration (ms) ---'};
        stats_lines(ms, 'ms');
        {''};
        {'--- Node Count ---'};
        stats_lines(nc, '');
    ];
    text(ax, 0.05, 0.95, txt, 'Units','normalized','VerticalAlignment','top', ...
         'Color',[0.15 0.15 0.15], 'FontSize',9, 'FontName','Monospaced');

    sgtitle(fig, sprintf('Assignment Solver  |  %s', label), 'FontSize',11);

    fprintf('\n=== %s ===\n', label);
    fprintf('--- Solve Duration (ms) ---\n');
    print_stats(ms, 'ms');
    fprintf('--- Node Count ---\n');
    print_stats(nc, '');
end

% ============================================================
%  HELPERS
% ============================================================

function lines = stats_lines(data, unit)
    if ~isempty(unit), u = [' ' unit]; else, u = ''; end
    lines = {
        sprintf('N      : %d',           numel(data));
        sprintf('Mean   : %.3f%s',       mean(data),        u);
        sprintf('Median : %.3f%s',       median(data),      u);
        sprintf('Std    : %.3f%s',       std(data),         u);
        sprintf('Min    : %.3f%s',       min(data),         u);
        sprintf('Max    : %.3f%s',       max(data),         u);
        sprintf('P95    : %.3f%s',       prctile(data,95),  u);
        sprintf('P99    : %.3f%s',       prctile(data,99),  u);
    };
end

function print_stats(data, unit)
    if ~isempty(unit), u = [' ' unit]; else, u = ''; end
    fprintf('  N      : %d\n',        numel(data));
    fprintf('  Mean   : %.3f%s\n',    mean(data),       u);
    fprintf('  Median : %.3f%s\n',    median(data),     u);
    fprintf('  Std    : %.3f%s\n',    std(data),        u);
    fprintf('  Min    : %.3f%s\n',    min(data),        u);
    fprintf('  Max    : %.3f%s\n',    max(data),        u);
    fprintf('  P95    : %.3f%s\n',    prctile(data,95), u);
    fprintf('  P99    : %.3f%s\n',    prctile(data,99), u);
end

function t = bag_timestamps(sel)
    % Returns elapsed seconds (double) from the first message in a bag selection
    tlist = sel.MessageList.Time;           % datetime array
    t     = seconds(seconds(tlist - tlist(1)));  % double elapsed seconds
end

function [vals, t] = crop(vals, t_raw, t_start, t_end)
    mask = t_raw >= t_start & t_raw <= t_end;
    if ~any(mask)
        warning('analyze_assignment_solvers: crop [%.1f, %.1f] s yields no samples (bag range: [%.1f, %.1f] s)', ...
            t_start, t_end, t_raw(1), t_raw(end));
        vals = [];
        t    = [];
        return;
    end
    vals = vals(mask);
    t    = t_raw(mask) - t_raw(find(mask,1));
end

function ax = light_ax(fig, rows, cols, idx)
    ax = subplot(rows, cols, idx, 'Parent', fig);
    set(ax, 'Color',     [1.0  1.0  1.0 ], ...
            'XColor',    [0.15 0.15 0.15], ...
            'YColor',    [0.15 0.15 0.15], ...
            'GridColor', [0.8  0.8  0.8 ], ...
            'GridAlpha', 0.6, ...
            'FontSize',  7);
end