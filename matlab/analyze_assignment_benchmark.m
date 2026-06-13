% ============================================================
% analyze_assignment_benchmark.m
%
% Paper figure generation for the assignment solver benchmark report.
%
% Outputs:
%   figures/Assignment-Benchmark-Report/scenario_<N>_per_cycle.{png}
%
% USAGE
%   analyze_assignment_benchmark
%   analyze_assignment_benchmark(scenario)
%   analyze_assignment_benchmark(scenario, t_start, t_end)
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2026-06-13
% ============================================================

function analyze_assignment_benchmark(scenario, t_start, t_end)
    if nargin < 1 || isempty(scenario), scenario = 2; end
    if nargin < 2, t_start = -inf; end
    if nargin < 3, t_end   =  inf; end

    script_dir = fileparts(mfilename('fullpath'));
    addpath(script_dir);

    style = analysis_common('style');
    out_dir = fullfile(script_dir, 'figures', 'Assignment-Benchmark-Report');
    if ~exist(out_dir, 'dir')
        mkdir(out_dir);
    end

    recordings_dir = fullfile(script_dir, '..', 'recordings');

    methods = method_registry(style);
    data = benchmark_data(scenario, recordings_dir, methods);
    plot_per_cycle_comparison(data, style, out_dir, scenario, t_start, t_end);
end

% ============================================================
%  PLOTS
% ============================================================

function plot_per_cycle_comparison(data, style, out_dir, scenario, t_start, t_end)
    n_methods = numel(data);
    n_rows = 2 * n_methods;

    fig = paper_figure(style, style.double_width, style.tall_height * (n_rows / 2));

    for m = 1:n_methods
        item = data(m);
        t_dur = item.t_duration(:);
        t_cnt = item.t_count(:);
        ms = item.duration_ms(:);
        nc = item.node_count(:);
        ms_win = ms(trim_mask(t_dur, t_start, t_end));
        nc_win = nc(trim_mask(t_cnt, t_start, t_end));
        [x_lo, x_hi] = trim_limits([t_dur; t_cnt], t_start, t_end);

        row_dur = 2 * m - 1;
        row_cnt = 2 * m;

        ax = analysis_common('axis', fig, n_rows, 1, row_dur);
        hold(ax, 'on'); grid(ax, 'on');
        plot(ax, t_dur, ms, '-', 'Color', style.orange, 'LineWidth', style.line_width);
        plot_stat_lines(ax, ms_win, style.orange);
        set_padded_ylim(ax, ms_win);
        xlim(ax, [x_lo, x_hi]);
        ylabel(ax, '$t_{solve}\,[ms]$', 'Interpreter', 'latex');
        title(ax, item.name, 'Interpreter', 'latex');

        ax = analysis_common('axis', fig, n_rows, 1, row_cnt);
        hold(ax, 'on'); grid(ax, 'on');
        plot(ax, t_cnt, nc, '-', 'Color', style.blue, 'LineWidth', style.line_width);
        plot_stat_lines(ax, nc_win, style.blue);
        set_padded_ylim(ax, nc_win);
        xlim(ax, [x_lo, x_hi]);
        ylabel(ax, '$N_{nodes}$', 'Interpreter', 'latex');
        if m == n_methods
            xlabel(ax, '$time\,[s]$', 'Interpreter', 'latex');
        end
    end

    export_report_figure(fig, out_dir, sprintf('scenario_%d_per_cycle', scenario));
end

% ============================================================
%  DATA
% ============================================================

function methods = method_registry(style)
    methods = struct( ...
        'suffix', {'BB', 'Exhaustive'}, ...
        'name',   {'Branch-and-Bound', 'Exhaustive search'});
end

function data = benchmark_data(scenario, recordings_dir, methods)
    data = struct( ...
        'suffix', {}, 'name', {}, ...
        'duration_ms', {}, 'node_count', {}, ...
        't_duration', {}, 't_count', {});

    for m = 1:numel(methods)
        suffix = methods(m).suffix;
        bag_name = sprintf('Assignment-Benchmark-%d-%s', scenario, suffix);
        bag_path = fullfile(recordings_dir, bag_name, sprintf('%s_0.mcap', bag_name));

        if ~isfile(bag_path)
            warning('analyze_assignment_benchmark:MissingBag', ...
                'Skipping %s — bag not found at %s', methods(m).name, bag_path);
            continue;
        end

        series = load_bag(bag_path);

        item.suffix = suffix;
        item.name = methods(m).name;
        item.duration_ms = series.duration_ms;
        item.node_count = series.node_count;
        item.t_duration = series.t_duration;
        item.t_count = series.t_count;
        data(end+1) = item;
    end

    if isempty(data)
        error('analyze_assignment_benchmark:NoData', ...
            'No benchmark bags found for scenario %d under %s', scenario, recordings_dir);
    end
end

function data = load_bag(bag_path)
    bag = ros2bagreader(bag_path);

    dur_topic = '/flychams/coordinator/assignment_solve_duration';
    cnt_topic = '/flychams/coordinator/assignment_node_count';

    dur_sel          = select(bag, 'Topic', dur_topic);
    dur_msgs         = readMessages(dur_sel);
    data.duration_ms = cellfun(@(m) double(m.data), dur_msgs);
    data.t_duration  = bag_timestamps(dur_sel);

    cnt_sel         = select(bag, 'Topic', cnt_topic);
    cnt_msgs        = readMessages(cnt_sel);
    data.node_count = cellfun(@(m) double(m.data), cnt_msgs);
    data.t_count    = bag_timestamps(cnt_sel);
end

% ============================================================
%  HELPERS
% ============================================================

function fig = paper_figure(style, width, height)
    fig = figure('NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Units', 'inches', 'Position', [0, 0, width, height]);
    set(fig, 'DefaultAxesFontSize', style.axis_font_size);
end

function plot_stat_lines(ax, values, color)
    values = values(:);
    values = values(~isnan(values));
    if isempty(values)
        return;
    end
    yline(ax, min(values),  '--',  'Color', color, 'LineWidth', 1.0);
    yline(ax, mean(values), '--', 'Color', color, 'LineWidth', 1.0);
    yline(ax, max(values), '--', 'Color', color, 'LineWidth', 1.0);
end

function set_padded_ylim(ax, values, pad_fraction)
    if nargin < 3, pad_fraction = 0.10; end
    values = values(:);
    values = values(~isnan(values));
    if isempty(values)
        return;
    end
    ymin = min(values);
    ymax = max(values);
    span = ymax - ymin;
    if span == 0
        pad = max(abs(ymin) * pad_fraction, 1.0);
    else
        pad = pad_fraction * span;
    end
    ylim(ax, [ymin - pad, ymax + pad]);
end

function export_report_figure(fig, out_dir, name)
    set(fig, 'PaperPositionMode', 'auto');
    drawnow;
    exportgraphics(fig, fullfile(out_dir, sprintf('%s.png', name)), 'Resolution', 300);
end

function mask = trim_mask(t, t_start, t_end)
    mask = true(size(t));
    if isfinite(t_start), mask = mask & (t >= t_start); end
    if isfinite(t_end),   mask = mask & (t <= t_end);   end
end

function [lo, hi] = trim_limits(t, t_start, t_end)
    t = t(:);
    if isempty(t)
        lo = 0;
        hi = 1;
        return;
    end
    lo = t_start;
    hi = t_end;
    if ~isfinite(lo), lo = min(t); end
    if ~isfinite(hi), hi = max(t); end
end

function t = bag_timestamps(sel)
    tlist = sel.MessageList.Time;
    t     = seconds(seconds(tlist - tlist(1)));
end
