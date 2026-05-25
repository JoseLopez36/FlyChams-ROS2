% ============================================================
% plot_trajectories.m
%
% Spatial trajectory visualisation for a FlyChams recording.
%
% Produces three figures:
%   Fig 1 — 2D top-down view: agents + targets + cluster circles
%   Fig 2 — 3D view: same data in XYZ space
%   Fig 3 — Per-agent distance-to-goal over time (tracking error proxy)
%
% USAGE
%   plot_trajectories(rec)
%   plot_trajectories(rec, 'SaveDir', '/path/to/figs')
%
% INPUT
%   rec       – struct returned by load_recording()
%
% PARAMETERS (name-value)
%   'SaveDir'       ''         If non-empty, saves PNG of each figure here
%   'Decimate'      10         Plot every N-th position sample (speed)
%   'TrailAlpha'    0.45       Opacity of trajectory lines
%   'ShowSetpoints' true       Overlay position setpoints on agent paths
%   'ShowClusters'  true       Draw Welzl enclosing circles per cluster
%   'ShowArrows'    false      Velocity arrows along agent path (slow)
%   'FigSize'       [1400 700] Figure width × height (px)
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-05-24
% ============================================================

function plot_trajectories(rec, varargin)

    % --------------------------------------------------------
    % Options
    % --------------------------------------------------------
    p = inputParser;
    addRequired(p,  'rec');
    addParameter(p, 'SaveDir',       '',         @ischar);
    addParameter(p, 'Decimate',      10,         @isnumeric);
    addParameter(p, 'TrailAlpha',    0.45,       @isnumeric);
    addParameter(p, 'ShowSetpoints', true,       @islogical);
    addParameter(p, 'ShowClusters',  true,       @islogical);
    addParameter(p, 'ShowArrows',    false,      @islogical);
    addParameter(p, 'FigSize',       [1400 700], @isnumeric);
    parse(p, rec, varargin{:});
    opt = p.Results;

    % --------------------------------------------------------
    % Colour palettes (fixed, consistent with Foxglove layout)
    % --------------------------------------------------------
    agent_cmap   = lines(max(rec.meta.n_agents,   1));
    target_cmap  = cool(max(rec.meta.n_targets,   1));
    cluster_cmap = autumn(max(rec.meta.n_clusters, 1));

    dec = opt.Decimate;

    % ============================================================
    %  FIG 1 — 2D top-down
    % ============================================================
    fig1 = make_figure('FlyChams — Trajectories (Top-down)', opt.FigSize);
    ax1 = setup_dark_axes(fig1, 1, 1, 1);
    hold(ax1, 'on'); grid(ax1, 'on'); axis(ax1, 'equal');
    xlabel(ax1, 'X (m)', 'Color', [0.9 0.9 0.9]);
    ylabel(ax1, 'Y (m)', 'Color', [0.9 0.9 0.9]);
    title(ax1, sprintf('Top-down  |  %s  |  %.0f s', ...
        rec.meta.recording_path, rec.meta.duration_s), ...
        'Color', [0.95 0.95 0.95], 'Interpreter', 'none', 'FontSize', 10);

    legend_handles = [];
    legend_labels  = {};

    % --- Cluster circles (background layer) ---
    if opt.ShowClusters
        cids = fieldnames(rec.clusters);
        for ci = 1:numel(cids)
            cdat = rec.clusters.(cids{ci});
            if isempty(cdat.center), continue; end
            col = cluster_cmap(ci,:);
            % Draw the time-mean enclosing circle
            cx_mean = mean(cdat.center(:,1));
            cy_mean = mean(cdat.center(:,2));
            r_mean  = mean(cdat.radius);
            ang = linspace(0, 2*pi, 180);
            plot(ax1, cx_mean + r_mean*cos(ang), cy_mean + r_mean*sin(ang), ...
                 '--', 'Color', [col, 0.4], 'LineWidth', 1.2);
            % Centroid path
            idx = 1:dec:size(cdat.center,1);
            h = plot(ax1, cdat.center(idx,1), cdat.center(idx,2), '-', ...
                 'Color', [col, 0.55], 'LineWidth', 1.0);
            plot(ax1, cdat.center(1,1), cdat.center(1,2), 'd', ...
                 'Color', col, 'MarkerFaceColor', col, 'MarkerSize', 7);
            text(ax1, cx_mean, cy_mean + r_mean + 0.5, cids{ci}, ...
                 'Color', col, 'FontSize', 7, 'HorizontalAlignment', 'center');
            legend_handles(end+1) = h; %#ok<AGROW>
            legend_labels{end+1}  = cids{ci}; %#ok<AGROW>
        end
    end

    % --- Target paths ---
    tids = fieldnames(rec.targets);
    for ti = 1:numel(tids)
        tdat = rec.targets.(tids{ti});
        if isempty(tdat.position), continue; end
        col = target_cmap(ti,:);
        idx = 1:dec:size(tdat.position,1);
        h = plot(ax1, tdat.position(idx,1), tdat.position(idx,2), '-', ...
             'Color', [col, opt.TrailAlpha], 'LineWidth', 0.9);
        plot(ax1, tdat.position(1,1), tdat.position(1,2), 'o', ...
             'Color', col, 'MarkerFaceColor', col, 'MarkerSize', 5);
        legend_handles(end+1) = h; %#ok<AGROW>
        legend_labels{end+1}  = tids{ti}; %#ok<AGROW>
    end

    % --- Agent paths ---
    aids = fieldnames(rec.agents);
    for ai = 1:numel(aids)
        adat = rec.agents.(aids{ai});
        if isempty(adat.position), continue; end
        col = agent_cmap(ai,:);
        idx = 1:dec:size(adat.position,1);
        h = plot(ax1, adat.position(idx,1), adat.position(idx,2), '-', ...
             'Color', col, 'LineWidth', 1.8);
        plot(ax1, adat.position(1,1), adat.position(1,2), '^', ...
             'Color', col, 'MarkerFaceColor', col, 'MarkerSize', 8);
        text(ax1, adat.position(1,1)+0.3, adat.position(1,2)+0.3, aids{ai}, ...
             'Color', col, 'FontSize', 8);

        if opt.ShowSetpoints && ~isempty(adat.setpoint)
            plot(ax1, adat.setpoint(idx,1), adat.setpoint(idx,2), ':', ...
                 'Color', [col, 0.5], 'LineWidth', 1.0);
        end

        if opt.ShowArrows
            step = max(1, round(size(adat.position,1)/20));
            idx_a = 1:step:size(adat.position,1)-1;
            dx = diff(adat.position(idx_a,1));
            dy = diff(adat.position(idx_a,2));
            quiver(ax1, adat.position(idx_a,1), adat.position(idx_a,2), ...
                   dx, dy, 0.5, 'Color', col, 'MaxHeadSize', 3);
        end

        legend_handles(end+1) = h; %#ok<AGROW>
        legend_labels{end+1}  = aids{ai}; %#ok<AGROW>
    end

    if ~isempty(legend_handles)
        legend(ax1, legend_handles, legend_labels, 'TextColor', [0.85 0.85 0.85], ...
               'Color', [0.18 0.18 0.20], 'EdgeColor', [0.4 0.4 0.4], ...
               'Location', 'best', 'FontSize', 8);
    end

    maybe_save(fig1, opt.SaveDir, 'trajectories_2d.png');

    % ============================================================
    %  FIG 2 — 3D view
    % ============================================================
    fig2 = make_figure('FlyChams — Trajectories (3D)', opt.FigSize);
    ax2 = setup_dark_axes(fig2, 1, 1, 1);
    hold(ax2, 'on'); grid(ax2, 'on');
    xlabel(ax2, 'X (m)', 'Color', [0.9 0.9 0.9]);
    ylabel(ax2, 'Y (m)', 'Color', [0.9 0.9 0.9]);
    zlabel(ax2, 'Z (m)', 'Color', [0.9 0.9 0.9]);
    title(ax2, '3D Trajectories', 'Color', [0.95 0.95 0.95], 'FontSize', 10);
    view(ax2, 45, 30);

    for ti = 1:numel(tids)
        tdat = rec.targets.(tids{ti});
        if isempty(tdat.position), continue; end
        col = target_cmap(ti,:);
        idx = 1:dec:size(tdat.position,1);
        plot3(ax2, tdat.position(idx,1), tdat.position(idx,2), tdat.position(idx,3), ...
              '-', 'Color', [col, opt.TrailAlpha], 'LineWidth', 0.9);
        plot3(ax2, tdat.position(1,1), tdat.position(1,2), tdat.position(1,3), ...
              'o', 'Color', col, 'MarkerFaceColor', col, 'MarkerSize', 5);
    end

    for ai = 1:numel(aids)
        adat = rec.agents.(aids{ai});
        if isempty(adat.position), continue; end
        col = agent_cmap(ai,:);
        idx = 1:dec:size(adat.position,1);
        plot3(ax2, adat.position(idx,1), adat.position(idx,2), adat.position(idx,3), ...
              '-', 'Color', col, 'LineWidth', 1.8);
        plot3(ax2, adat.position(1,1), adat.position(1,2), adat.position(1,3), ...
              '^', 'Color', col, 'MarkerFaceColor', col, 'MarkerSize', 8);
    end

    maybe_save(fig2, opt.SaveDir, 'trajectories_3d.png');

    % ============================================================
    %  FIG 3 — Distance-to-goal per agent over time
    % ============================================================
    fig3 = make_figure('FlyChams — Distance to Goal', opt.FigSize);
    n_agents = numel(aids);
    rows = ceil(n_agents / 2);
    cols = min(n_agents, 2);

    for ai = 1:n_agents
        adat = rec.agents.(aids{ai});
        ax = setup_dark_axes(fig3, rows, cols, ai);
        hold(ax, 'on'); grid(ax, 'on');
        col = agent_cmap(ai,:);
        title(ax, aids{ai}, 'Color', [0.95 0.95 0.95], 'FontSize', 9);
        xlabel(ax, 'Time (s)', 'Color', [0.8 0.8 0.8], 'FontSize', 8);
        ylabel(ax, 'Dist-to-goal (m)', 'Color', [0.8 0.8 0.8], 'FontSize', 8);

        if ~isempty(adat.distance_to_goal)
            plot(ax, adat.t_pos, adat.distance_to_goal, '-', 'Color', col, 'LineWidth', 1.2);
            yline(ax, mean(adat.distance_to_goal), '--', 'Color', [col, 0.6], 'LineWidth', 1.0);
            text(ax, adat.t_pos(end)*0.02, mean(adat.distance_to_goal)*1.05, ...
                 sprintf('mean=%.1f m', mean(adat.distance_to_goal)), ...
                 'Color', [0.8 0.8 0.8], 'FontSize', 7);
        end
    end
    sgtitle(fig3, 'Distance to Assigned Goal', 'Color', [0.95 0.95 0.95], 'FontSize', 11);

    maybe_save(fig3, opt.SaveDir, 'distance_to_goal.png');
end

% ============================================================
%  HELPERS
% ============================================================

function fig = make_figure(name, sz)
    fig = figure('Name', name, 'NumberTitle', 'off', ...
                 'Color', [0.12 0.12 0.14], ...
                 'Position', [100, 100, sz(1), sz(2)]);
end

function ax = setup_dark_axes(fig, rows, cols, idx)
    ax = subplot(rows, cols, idx, 'Parent', fig);
    set(ax, 'Color',     [0.15 0.15 0.18], ...
            'XColor',    [0.8  0.8  0.8 ], ...
            'YColor',    [0.8  0.8  0.8 ], ...
            'ZColor',    [0.8  0.8  0.8 ], ...
            'GridColor', [0.35 0.35 0.35], ...
            'GridAlpha', 0.4);
end

function maybe_save(fig, save_dir, fname)
    if isempty(save_dir), return; end
    if ~exist(save_dir, 'dir'), mkdir(save_dir); end
    exportgraphics(fig, fullfile(save_dir, fname), 'Resolution', 150);
    fprintf('Saved: %s\n', fullfile(save_dir, fname));
end