% ============================================================
% analyze_assignment_benchmark.m
%
% Paper figure generation for the assignment solver benchmark report.
%
% Outputs:
%   figures/Assignment-Benchmark-Report/scenario_<N>_per_cycle.{png}
%   stats/Assignment-Benchmark-Report/scenario_<N>_per_cycle.txt
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

    methods = method_registry();
    data = benchmark_data(scenario, recordings_dir, methods);
    report_window_stats(data, scenario, t_start, t_end, script_dir);
    plot_per_cycle_comparison(data, style, out_dir, scenario, t_start, t_end);
end

% ============================================================
%  REPORT
% ============================================================

function report_window_stats(data, scenario, t_start, t_end, script_dir)
    all_t = [];
    for m = 1:numel(data)
        all_t = [all_t; data(m).t_duration(:); data(m).t_count(:)]; %#ok<AGROW>
    end
    [x_lo, x_hi] = analysis_common('trim_limits', all_t, t_start, t_end);

    lines = {
        sprintf('Scenario %d', scenario);
        sprintf('Time window: [%.3f, %.3f] s', x_lo, x_hi);
        ''
    };

    for m = 1:numel(data)
        item = data(m);
        ms_win = analysis_common('values_in_window', item.duration_ms, item.t_duration, t_start, t_end);
        nc_win = analysis_common('values_in_window', item.node_count, item.t_count, t_start, t_end);

        lines = [lines; ...
            {item.name}; ...
            {'  --- Solve Duration (ms) ---'}; ...
            analysis_common('min_mean_max_lines', ms_win, 'ms'); ...
            {'  --- Node Count ---'}; ...
            analysis_common('min_mean_max_lines', nc_win, ''); ...
            {''}];
    end

    out_stats_dir = fullfile(script_dir, 'stats', 'Assignment-Benchmark-Report');
    analysis_common('export_stats', out_stats_dir, '', sprintf('scenario_%d_per_cycle', scenario), lines);

    fprintf('\n=== Assignment benchmark — scenario %d ===\n', scenario);
    fprintf('%s\n', strjoin(lines, newline));
end

% ============================================================
%  PLOTS
% ============================================================

function plot_per_cycle_comparison(data, style, out_dir, scenario, t_start, t_end)
    n_methods = numel(data);
    n_rows = 2 * n_methods;

    fig = analysis_common('paper_figure', style, style.double_width, style.tall_height * (n_rows / 2));

    for m = 1:n_methods
        item = data(m);
        t_dur = item.t_duration(:);
        t_cnt = item.t_count(:);
        ms = item.duration_ms(:);
        nc = item.node_count(:);
        ms_win = analysis_common('values_in_window', ms, t_dur, t_start, t_end);
        nc_win = analysis_common('values_in_window', nc, t_cnt, t_start, t_end);

        row_dur = 2 * m - 1;
        row_cnt = 2 * m;

        ax = analysis_common('axis', fig, n_rows, 1, row_dur);
        hold(ax, 'on'); grid(ax, 'on');
        plot(ax, t_dur, ms, '-', 'Color', style.orange, 'LineWidth', style.line_width);
        analysis_common('plot_min_mean_max', ax, ms_win, style.orange);
        analysis_common('padded_ylim', ax, ms_win);
        analysis_common('apply_trim_xlim', ax, t_dur, t_start, t_end);
        ylabel(ax, '$t_{solve}\,[ms]$', 'Interpreter', 'latex');
        title(ax, item.name, 'Interpreter', 'latex');

        ax = analysis_common('axis', fig, n_rows, 1, row_cnt);
        hold(ax, 'on'); grid(ax, 'on');
        plot(ax, t_cnt, nc, '-', 'Color', style.blue, 'LineWidth', style.line_width);
        analysis_common('plot_min_mean_max', ax, nc_win, style.blue);
        analysis_common('padded_ylim', ax, nc_win);
        analysis_common('apply_trim_xlim', ax, t_cnt, t_start, t_end);
        ylabel(ax, '$N_{nodes}$', 'Interpreter', 'latex');
        if m == n_methods
            xlabel(ax, '$time\,[s]$', 'Interpreter', 'latex');
        end
    end

    analysis_common('export_report_figure', fig, out_dir, sprintf('scenario_%d_per_cycle', scenario));
end

% ============================================================
%  DATA
% ============================================================

function methods = method_registry()
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
    data.t_duration  = analysis_common('bag_timestamps', dur_sel);

    cnt_sel         = select(bag, 'Topic', cnt_topic);
    cnt_msgs        = readMessages(cnt_sel);
    data.node_count = cellfun(@(m) double(m.data), cnt_msgs);
    data.t_count    = analysis_common('bag_timestamps', cnt_sel);
end
