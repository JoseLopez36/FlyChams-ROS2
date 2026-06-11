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
    stem = analysis_common('bag_label', bag_path);
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
    style = analysis_common('style');
    out_fig_dir = analysis_common('figures_dir');
    out_stats_dir = analysis_common('stats_dir');
    col_dur = style.orange;
    col_cnt = style.blue;

    ms = data.duration_ms(:);
    t  = data.t_duration(:);
    nc = data.node_count(:);
    tc = data.t_count(:);

    fig = figure('Name', sprintf('Assignment Solvers - %s', label), ...
                 'NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Units', 'inches', 'Position', [0, 0, style.double_width, style.tall_height]);

    ax = analysis_common('axis', fig, 2, 1, 1);
    hold(ax,'on'); grid(ax,'on');
    ylabel(ax, '$t_{solve}\,[ms]$', 'Interpreter','latex');
    plot(ax, t, ms, '-', 'Color', col_dur, 'LineWidth', style.line_width);
    yline(ax, mean(ms),       '--', 'Color', col_dur,   'LineWidth', 1.0);
    yline(ax, prctile(ms,99), ':',  'Color', style.red, 'LineWidth', 1.0);

    ax = analysis_common('axis', fig, 2, 1, 2);
    hold(ax,'on'); grid(ax,'on');
    xlabel(ax, '$time\,[s]$', 'Interpreter','latex');
    ylabel(ax, '$N_{nodes}$', 'Interpreter','latex');
    plot(ax, tc, nc, '-', 'Color', col_cnt, 'LineWidth', style.line_width);
    yline(ax, mean(nc),       '--', 'Color', col_cnt,   'LineWidth', 1.0);
    yline(ax, prctile(nc,99), ':',  'Color', style.red, 'LineWidth', 1.0);
    analysis_common('export_figure', fig, out_fig_dir, label, 'assignment_solvers');

    txt = [
        {'--- Solve Duration (ms) ---'};
        analysis_common('stats_lines', ms, 'ms');
        {''};
        {'--- Node Count ---'};
        analysis_common('stats_lines', nc, '');
    ];
    analysis_common('export_stats', out_stats_dir, label, 'assignment_solvers', txt);

    fprintf('\n=== %s ===\n', label);
    fprintf('%s\n', strjoin(txt, newline));
end

% ============================================================
%  HELPERS
% ============================================================

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