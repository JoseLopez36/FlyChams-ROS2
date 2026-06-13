% ============================================================
% analyze_heterogeneous_benchmark.m
%
% Paper figure generation for the heterogeneous fleet benchmark report.
%
% Outputs:
%   figures/Heterogeneous-Benchmark-Report/observation_quality.{png,pdf}
%   figures/Heterogeneous-Benchmark-Report/deployment_efficiency_pareto.{png,pdf}
%   figures/Heterogeneous-Benchmark-Report/control_stability.{png,pdf}
%   figures/Heterogeneous-Benchmark-Report/compute_latency.{png,pdf}
%   figures/Heterogeneous-Benchmark-Report/specialization_evidence.{png,pdf}
%
% USAGE
%   analyze_heterogeneous_benchmark
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2026-06-08
% ============================================================

function analyze_heterogeneous_benchmark()
    script_dir = fileparts(mfilename('fullpath'));
    addpath(script_dir);

    style = analysis_common('style');
    out_dir = fullfile(script_dir, 'figures', 'Heterogeneous-Benchmark-Report');
    if ~exist(out_dir, 'dir')
        mkdir(out_dir);
    end

    data = benchmark_data();
    plot_observation_quality(data, style, out_dir);
    plot_deployment_efficiency(data, style, out_dir);
    plot_control_stability(data, style, out_dir);
    plot_compute_latency(data, style, out_dir);
end

% ============================================================
%  PLOTS
% ============================================================

function plot_observation_quality(data, style, out_dir)
    clusters = unique_stable({data.cluster});
    fleets = unique_stable({data.fleet});

    zoom_p95 = metric_grid(data, clusters, fleets, 'zoom_p95');
    size_mean = metric_grid(data, clusters, fleets, 'size_mean');

    fig = analysis_common('paper_figure', style, style.double_width, style.tall_height);

    ax = analysis_common('axis', fig, 2, 1, 1);
    bar(ax, categorical(clusters), zoom_p95, 'grouped');
    grid(ax, 'on');
    ylabel(ax, '$\tilde\nu_{95}$', 'Interpreter','latex');
    ylim(ax, [0 1.10]);
    title(ax, 'Zoom factor (p95)', 'Interpreter','latex');
    legend(ax, fleets, 'Location','northoutside', 'Orientation','horizontal', 'Interpreter','latex');

    ax = analysis_common('axis', fig, 2, 1, 2);
    bar(ax, categorical(clusters), size_mean, 'grouped');
    grid(ax, 'on');
    ylabel(ax, '$\bar{s}\,[pix]$', 'Interpreter','latex');
    ylim(ax, [0 230]);
    yline(ax, 216, '--', 'Color', style.green, 'LineWidth', 1.0, 'Interpreter','latex');
    title(ax, 'Apparent target size (mean)', 'Interpreter','latex');

    analysis_common('export_report_figure', fig, out_dir, 'observation_quality');
end

function plot_deployment_efficiency(data, style, out_dir)
    clusters = unique_stable({data.cluster});
    fleets = unique_stable({data.fleet});
    markers = {'o', 's', '^'};

    fig = analysis_common('paper_figure', style, style.double_width, style.short_height);
    ax = analysis_common('axis', fig, 1, 1, 1);
    hold(ax, 'on'); grid(ax, 'on');

    for f = 1:numel(fleets)
        idx = strcmp({data.fleet}, fleets{f});
        scatter(ax, [data(idx).total_travel], [data(idx).zoom_p95], ...
            70 + 35 * [data(idx).agents], markers{f}, 'filled', ...
            'DisplayName', fleets{f});
    end

    xlabel(ax, '$d\,[m]$', 'Interpreter','latex');
    ylabel(ax, '$\tilde\nu_{95}$', 'Interpreter','latex');
    ylim(ax, [0.25 1.08]);
    title(ax, 'Deployment efficiency (travel distance vs. zoom)', 'Interpreter','latex');
    legend(ax, 'Location','southeast', 'Interpreter','latex');

    analysis_common('export_report_figure', fig, out_dir, 'deployment_efficiency');
end

function plot_control_stability(data, style, out_dir)
    clusters = unique_stable({data.cluster});
    fleets = unique_stable({data.fleet});

    goal_mean = metric_grid(data, clusters, fleets, 'goal_mean');
    travel_std = metric_grid(data, clusters, fleets, 'travel_std');

    fig = analysis_common('paper_figure', style, style.double_width, style.tall_height);

    ax = analysis_common('axis', fig, 2, 1, 1);
    bar(ax, categorical(clusters), goal_mean, 'grouped');
    grid(ax, 'on');
    ylabel(ax, '$\bar{e}_{goal}\,[m]$', 'Interpreter','latex');
    title(ax, 'Setpoint tracking error', 'Interpreter','latex');
    legend(ax, fleets, 'Location','northoutside', 'Orientation','horizontal', 'Interpreter','latex');

    ax = analysis_common('axis', fig, 2, 1, 2);
    bar(ax, categorical(clusters), travel_std, 'grouped');
    grid(ax, 'on');
    ylabel(ax, '$\sigma_d\,[m]$', 'Interpreter','latex');
    title(ax, 'Inter-agent travel balance', 'Interpreter','latex');

    analysis_common('export_report_figure', fig, out_dir, 'control_stability');
end

function plot_compute_latency(data, style, out_dir)
    clusters = unique_stable({data.cluster});
    fleets = unique_stable({data.fleet});
    assign_p95 = metric_grid(data, clusters, fleets, 'assign_p95');

    fig = analysis_common('paper_figure', style, style.double_width, style.short_height);
    ax = analysis_common('axis', fig, 1, 1, 1);
    bar(ax, categorical(clusters), assign_p95, 'grouped');
    grid(ax, 'on');
    ylabel(ax, '$t_{95}\,[ms]$', 'Interpreter','latex');
    title(ax, 'Assignment solver latency (p95)', 'Interpreter','latex');
    legend(ax, fleets, 'Location','northoutside', 'Orientation','horizontal', 'Interpreter','latex');

    analysis_common('export_report_figure', fig, out_dir, 'compute_latency');
end

% ============================================================
%  DATA
% ============================================================

function data = benchmark_data()
    data = struct( ...
        'cluster', {}, 'fleet', {}, 'agents', {}, 'zoom_mean', {}, 'zoom_p95', {}, ...
        'size_mean', {}, 'total_travel', {}, 'mean_travel', {}, 'travel_std', {}, ...
        'goal_mean', {}, 'assign_mean', {}, 'assign_p95', {});

    data(end+1) = row('Dispersed',     'Type A',        3, 0.263, 0.364, 216.000, 402.171, 134.057, 21.780, 0.929, 590.105, 795.614);
    data(end+1) = row('Dispersed',     'Type B',        3, 0.657, 1.000, 213.265, 342.030, 114.010, 18.133, 0.734, 505.543, 697.445);
    data(end+1) = row('Dispersed',     'Heterogeneous', 2, 0.596, 1.000, 207.098, 208.450, 104.225, 25.912, 0.633, 706.244, 931.371);
    data(end+1) = row('Concentrated',  'Type A',        3, 0.384, 0.582, 216.000, 311.813, 103.938, 12.582, 0.736, 466.736, 570.690);
    data(end+1) = row('Concentrated',  'Type B',        3, 0.966, 1.000, 169.915, 289.001,  96.334,  9.363, 0.606, 432.010, 798.166);
    data(end+1) = row('Concentrated',  'Heterogeneous', 2, 0.747, 1.000, 171.281, 178.177,  89.088,  4.538, 0.567, 404.837, 494.498);
    data(end+1) = row('Mixed',         'Type A',        3, 0.337, 0.589, 216.000, 336.808, 112.269, 12.310, 0.720, 564.502, 765.673);
    data(end+1) = row('Mixed',         'Type B',        3, 0.807, 1.000, 183.631, 308.894, 102.965,  5.360, 0.702, 461.083, 570.633);
    data(end+1) = row('Mixed',         'Heterogeneous', 2, 0.702, 1.000, 202.654, 224.228, 112.114,  8.849, 0.600, 329.665, 418.920);
end

function item = row(cluster, fleet, agents, zoom_mean, zoom_p95, size_mean, total_travel, mean_travel, travel_std, goal_mean, assign_mean, assign_p95)
    item.cluster = cluster;
    item.fleet = fleet;
    item.agents = agents;
    item.zoom_mean = zoom_mean;
    item.zoom_p95 = zoom_p95;
    item.size_mean = size_mean;
    item.total_travel = total_travel;
    item.mean_travel = mean_travel;
    item.travel_std = travel_std;
    item.goal_mean = goal_mean;
    item.assign_mean = assign_mean;
    item.assign_p95 = assign_p95;
end

% ============================================================
%  HELPERS
% ============================================================

function values = metric_grid(data, clusters, fleets, field)
    values = nan(numel(clusters), numel(fleets));
    for c = 1:numel(clusters)
        for f = 1:numel(fleets)
            idx = strcmp({data.cluster}, clusters{c}) & strcmp({data.fleet}, fleets{f});
            values(c, f) = data(idx).(field);
        end
    end
end

function values = unique_stable(values)
    [~, idx] = unique(values, 'stable');
    values = values(sort(idx));
end

function idx = cluster_index(clusters, cluster)
    idx = find(strcmp(clusters, cluster), 1);
end