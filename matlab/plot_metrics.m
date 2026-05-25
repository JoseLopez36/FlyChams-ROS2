% ============================================================
% plot_metrics.m
%
% Time-series dashboard for all FlyChams operator metrics.
%
% Produces four figures:
%   Fig 1 — Mission overview  (counts, elapsed time)
%   Fig 2 — Per-agent panel   (speed, distance, dist-to-goal, zoom)
%   Fig 3 — Per-target panel  (speed, cumulative distance)
%   Fig 4 — Per-cluster panel (radius, speed, cumulative distance)
%
% USAGE
%   plot_metrics(rec)
%   plot_metrics(rec, 'SaveDir', '/path/to/figs')
%
% INPUT
%   rec  – struct returned by load_recording()
%
% PARAMETERS (name-value)
%   'SaveDir'    ''      If non-empty, saves PNG of each figure here
%   'FigSize'    [1400 900]
%   'Smooth'     5       Moving-average half-window (samples, 0 = off)
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-05-24
% ============================================================

function plot_metrics(rec, varargin)

    p = inputParser;
    addRequired(p,  'rec');
    addParameter(p, 'SaveDir', '',        @ischar);
    addParameter(p, 'FigSize', [1400 900], @isnumeric);
    addParameter(p, 'Smooth',  5,         @isnumeric);
    parse(p, rec, varargin{:});
    opt = p.Results;

    agent_cmap   = lines(max(rec.meta.n_agents,   1));
    target_cmap  = cool(max(rec.meta.n_targets,   1));
    cluster_cmap = autumn(max(rec.meta.n_clusters, 1));

    % ============================================================
    %  FIG 1 — Mission overview
    % ============================================================
    fig1 = make_figure('FlyChams — Mission Overview', opt.FigSize);

    % Row 1: active element counts
    ax = setup_dark_axes(fig1, 2, 2, 1);
    hold(ax,'on'); grid(ax,'on');
    title(ax, 'Active Elements', 'Color',[0.95 0.95 0.95]);
    xlabel(ax,'Time (s)','Color',[0.8 0.8 0.8],'FontSize',8);
    if ~isempty(rec.mission.t)
        plot(ax, rec.mission.t, rec.mission.total_agents,   '-',  'Color', [0.3 0.7 1.0], 'LineWidth',1.4, 'DisplayName','Agents');
        plot(ax, rec.mission.t, rec.mission.total_targets,  '--', 'Color', [1.0 0.6 0.2], 'LineWidth',1.4, 'DisplayName','Targets');
        plot(ax, rec.mission.t, rec.mission.total_clusters, ':',  'Color', [0.4 1.0 0.5], 'LineWidth',1.4, 'DisplayName','Clusters');
        legend(ax, 'TextColor',[0.85 0.85 0.85],'Color',[0.18 0.18 0.20],'EdgeColor',[0.4 0.4 0.4],'FontSize',8);
    end

    % Row 1: mission elapsed time
    ax = setup_dark_axes(fig1, 2, 2, 2);
    hold(ax,'on'); grid(ax,'on');
    title(ax,'Mission Elapsed Time','Color',[0.95 0.95 0.95]);
    xlabel(ax,'ROS stamp (s)','Color',[0.8 0.8 0.8],'FontSize',8);
    ylabel(ax,'Mission time (s)','Color',[0.8 0.8 0.8],'FontSize',8);
    if ~isempty(rec.mission.t)
        plot(ax, rec.mission.t, rec.mission.time, '-', 'Color',[0.7 0.7 1.0],'LineWidth',1.4);
    end

    % Row 2: assignment solve duration
    ax = setup_dark_axes(fig1, 2, 2, 3);
    hold(ax,'on'); grid(ax,'on');
    title(ax,'Assignment Solve Duration','Color',[0.95 0.95 0.95]);
    xlabel(ax,'Time (s)','Color',[0.8 0.8 0.8],'FontSize',8);
    ylabel(ax,'Duration (ms)','Color',[0.8 0.8 0.8],'FontSize',8);
    if ~isempty(rec.fleet.t)
        d = rec.fleet.assignment_solve_duration;
        plot(ax, rec.fleet.t, d, '-', 'Color',[1.0 0.4 0.4],'LineWidth',1.2);
        yline(ax, mean(d), '--', 'Color',[1.0 0.6 0.6],'LineWidth',1.0);
        add_stats_text(ax, d, rec.fleet.t);
    end

    % Row 2: total distance traveled per agent (bar summary)
    ax = setup_dark_axes(fig1, 2, 2, 4);
    hold(ax,'on'); grid(ax,'on');
    title(ax,'Total Distance Traveled (Agents)','Color',[0.95 0.95 0.95]);
    ylabel(ax,'Distance (m)','Color',[0.8 0.8 0.8],'FontSize',8);
    aids = fieldnames(rec.agents);
    dist_vals = zeros(1, numel(aids));
    for ai = 1:numel(aids)
        adat = rec.agents.(aids{ai});
        if ~isempty(adat.distance_traveled)
            dist_vals(ai) = adat.distance_traveled(end);
        end
    end
    b = bar(ax, dist_vals, 'FaceColor','flat');
    for ai = 1:numel(aids)
        b.CData(ai,:) = agent_cmap(ai,:);
    end
    set(ax, 'XTick', 1:numel(aids), 'XTickLabel', aids, 'XTickLabelRotation', 25);

    sgtitle(fig1, sprintf('Mission Overview  |  %.0f s', rec.meta.duration_s), ...
        'Color',[0.95 0.95 0.95],'FontSize',11);
    maybe_save(fig1, opt.SaveDir, 'metrics_mission.png');

    % ============================================================
    %  FIG 2 — Per-agent panel
    % ============================================================
    n_agents = numel(aids);
    if n_agents == 0, return; end
    fig2 = make_figure('FlyChams — Agent Metrics', opt.FigSize);
    n_cols = 4;  % speed | distance | dist-to-goal | zoom
    for ai = 1:n_agents
        adat = rec.agents.(aids{ai});
        col  = agent_cmap(ai,:);
        t    = adat.t_pos;

        % Speed
        ax = setup_dark_axes(fig2, n_agents, n_cols, (ai-1)*n_cols + 1);
        hold(ax,'on'); grid(ax,'on');
        ylabel(ax, aids{ai}, 'Color', col, 'FontWeight','bold','FontSize',8);
        if ai == 1, title(ax,'Speed (m/s)','Color',[0.95 0.95 0.95],'FontSize',9); end
        if ~isempty(adat.speed)
            s_sm = smooth_data(adat.speed, opt.Smooth);
            plot(ax, t, s_sm, '-', 'Color', col, 'LineWidth',1.2);
            add_stats_text(ax, adat.speed, t);
        end

        % Cumulative distance
        ax = setup_dark_axes(fig2, n_agents, n_cols, (ai-1)*n_cols + 2);
        hold(ax,'on'); grid(ax,'on');
        if ai == 1, title(ax,'Distance Traveled (m)','Color',[0.95 0.95 0.95],'FontSize',9); end
        if ~isempty(adat.distance_traveled)
            plot(ax, t, adat.distance_traveled, '-', 'Color', col, 'LineWidth',1.2);
        end

        % Distance to goal
        ax = setup_dark_axes(fig2, n_agents, n_cols, (ai-1)*n_cols + 3);
        hold(ax,'on'); grid(ax,'on');
        if ai == 1, title(ax,'Dist-to-goal (m)','Color',[0.95 0.95 0.95],'FontSize',9); end
        if ~isempty(adat.distance_to_goal)
            d_sm = smooth_data(adat.distance_to_goal, opt.Smooth);
            plot(ax, t, d_sm, '-', 'Color', col, 'LineWidth',1.2);
            add_stats_text(ax, adat.distance_to_goal, t);
        end

        % Zoom factors (all tracking units overlaid)
        ax = setup_dark_axes(fig2, n_agents, n_cols, (ai-1)*n_cols + 4);
        hold(ax,'on'); grid(ax,'on');
        if ai == 1, title(ax,'Zoom Factors','Color',[0.95 0.95 0.95],'FontSize',9); end
        if ~isempty(adat.zoom_factors)
            K = size(adat.zoom_factors, 2);
            for k = 1:K
                zf = smooth_data(adat.zoom_factors(:,k), opt.Smooth);
                plot(ax, t, zf, '-', 'Color', darken(col, 0.3*(k-1)), 'LineWidth', 1.0);
            end
        end
    end
    sgtitle(fig2,'Per-Agent Metrics','Color',[0.95 0.95 0.95],'FontSize',11);
    maybe_save(fig2, opt.SaveDir, 'metrics_agents.png');

    % ============================================================
    %  FIG 3 — Per-target panel
    % ============================================================
    tids = fieldnames(rec.targets);
    n_tgts = numel(tids);
    if n_tgts > 0
        fig3 = make_figure('FlyChams — Target Metrics', opt.FigSize);
        n_cols_t = 2;  % speed | distance
        for ti = 1:n_tgts
            tdat = rec.targets.(tids{ti});
            col  = target_cmap(ti,:);
            t    = tdat.t;

            ax = setup_dark_axes(fig3, n_tgts, n_cols_t, (ti-1)*n_cols_t + 1);
            hold(ax,'on'); grid(ax,'on');
            ylabel(ax, tids{ti}, 'Color', col, 'FontWeight','bold','FontSize',8);
            if ti == 1, title(ax,'Speed (m/s)','Color',[0.95 0.95 0.95],'FontSize',9); end
            if ~isempty(tdat.speed)
                plot(ax, t, smooth_data(tdat.speed, opt.Smooth), '-', 'Color', col, 'LineWidth',1.2);
                add_stats_text(ax, tdat.speed, t);
            end

            ax = setup_dark_axes(fig3, n_tgts, n_cols_t, (ti-1)*n_cols_t + 2);
            hold(ax,'on'); grid(ax,'on');
            if ti == 1, title(ax,'Distance Traveled (m)','Color',[0.95 0.95 0.95],'FontSize',9); end
            if ~isempty(tdat.distance_traveled)
                plot(ax, t, tdat.distance_traveled, '-', 'Color', col, 'LineWidth',1.2);
            end
        end
        sgtitle(fig3,'Per-Target Metrics','Color',[0.95 0.95 0.95],'FontSize',11);
        maybe_save(fig3, opt.SaveDir, 'metrics_targets.png');
    end

    % ============================================================
    %  FIG 4 — Per-cluster panel
    % ============================================================
    cids = fieldnames(rec.clusters);
    n_cls = numel(cids);
    if n_cls > 0
        fig4 = make_figure('FlyChams — Cluster Metrics', opt.FigSize);
        n_cols_c = 3;  % radius | speed | distance
        for ci = 1:n_cls
            cdat = rec.clusters.(cids{ci});
            col  = cluster_cmap(ci,:);
            t    = cdat.t;

            ax = setup_dark_axes(fig4, n_cls, n_cols_c, (ci-1)*n_cols_c + 1);
            hold(ax,'on'); grid(ax,'on');
            ylabel(ax, cids{ci}, 'Color', col, 'FontWeight','bold','FontSize',8);
            if ci == 1, title(ax,'Enclosing Radius (m)','Color',[0.95 0.95 0.95],'FontSize',9); end
            if ~isempty(cdat.radius)
                plot(ax, t, smooth_data(cdat.radius, opt.Smooth), '-', 'Color', col, 'LineWidth',1.2);
                add_stats_text(ax, cdat.radius, t);
            end

            ax = setup_dark_axes(fig4, n_cls, n_cols_c, (ci-1)*n_cols_c + 2);
            hold(ax,'on'); grid(ax,'on');
            if ci == 1, title(ax,'Centroid Speed (m/s)','Color',[0.95 0.95 0.95],'FontSize',9); end
            if ~isempty(cdat.speed)
                plot(ax, t, smooth_data(cdat.speed, opt.Smooth), '-', 'Color', col, 'LineWidth',1.2);
                add_stats_text(ax, cdat.speed, t);
            end

            ax = setup_dark_axes(fig4, n_cls, n_cols_c, (ci-1)*n_cols_c + 3);
            hold(ax,'on'); grid(ax,'on');
            if ci == 1, title(ax,'Centroid Distance (m)','Color',[0.95 0.95 0.95],'FontSize',9); end
            if ~isempty(cdat.distance_traveled)
                plot(ax, t, cdat.distance_traveled, '-', 'Color', col, 'LineWidth',1.2);
            end
        end
        sgtitle(fig4,'Per-Cluster Metrics','Color',[0.95 0.95 0.95],'FontSize',11);
        maybe_save(fig4, opt.SaveDir, 'metrics_clusters.png');
    end
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
            'GridColor', [0.35 0.35 0.35], ...
            'GridAlpha', 0.4, ...
            'FontSize',  7);
end

function add_stats_text(ax, data, t)
    if isempty(data), return; end
    mu  = mean(data);
    mx  = max(data);
    txt = sprintf('μ=%.2f  max=%.2f', mu, mx);
    xl = xlim(ax);
    yl = ylim(ax);
    text(ax, xl(1) + 0.02*(xl(2)-xl(1)), yl(2) - 0.06*(yl(2)-yl(1)), txt, ...
         'Color', [0.75 0.75 0.75], 'FontSize', 6, 'VerticalAlignment', 'top');
end

function s = smooth_data(x, hw)
    if hw <= 0 || numel(x) < 3
        s = x;
        return;
    end
    N = numel(x);
    s = zeros(N,1);
    for i = 1:N
        lo = max(1, i-hw);
        hi = min(N, i+hw);
        s(i) = mean(x(lo:hi));
    end
end

function c = darken(col, amount)
    c = max(0, col - amount);
end

function maybe_save(fig, save_dir, fname)
    if isempty(save_dir), return; end
    if ~exist(save_dir, 'dir'), mkdir(save_dir); end
    exportgraphics(fig, fullfile(save_dir, fname), 'Resolution', 150);
    fprintf('Saved: %s\n', fullfile(save_dir, fname));
end