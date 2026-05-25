% ============================================================
% plot_solver.m
%
% Position-solver and assignment-solver performance analysis.
%
% Sources:
%   /flychams/agent/AGENTID/position_solve_duration  (Float32, ms)
%   /flychams/operator/fleet_metrics                 (assignment_solve_duration, ms)
%   rec.agents.<ID>.pos_solve_ms  (already loaded by load_recording)
%   rec.fleet.assignment_solve_duration
%
% Produces two figures:
%   Fig 1 — Position solver  : time-series + histogram + CDF per agent
%   Fig 2 — Assignment solver: time-series + histogram + stats summary
%
% USAGE
%   plot_solver(rec)
%   plot_solver(rec, 'SaveDir', '/path/to/figs')
%
% PARAMETERS (name-value)
%   'SaveDir'       ''           Save PNGs here if non-empty
%   'FigSize'       [1400 700]
%   'Smooth'        5            Half-window for time-series smoothing
%   'NBins'         50           Histogram bin count
%   'WorstPct'      99           Percentile to mark as "worst-case" line
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-05-24
% ============================================================

function plot_solver(rec, varargin)

    p = inputParser;
    addRequired(p,  'rec');
    addParameter(p, 'SaveDir',  '',        @ischar);
    addParameter(p, 'FigSize',  [1400 700], @isnumeric);
    addParameter(p, 'Smooth',   5,         @isnumeric);
    addParameter(p, 'NBins',    50,        @isnumeric);
    addParameter(p, 'WorstPct', 99,        @isnumeric);
    parse(p, rec, varargin{:});
    opt = p.Results;

    aids = fieldnames(rec.agents);
    n_agents = numel(aids);
    agent_cmap = lines(max(n_agents, 1));

    % ============================================================
    %  FIG 1 — Position solver
    % ============================================================
    fig1 = make_figure('FlyChams — Position Solver Performance', opt.FigSize);

    % Layout: 3 columns — time-series | histogram | CDF
    % One row per agent
    if n_agents == 0
        close(fig1); return;
    end

    all_pos_ms = [];  % pooled across agents for summary stats

    for ai = 1:n_agents
        adat = rec.agents.(aids{ai});
        col  = agent_cmap(ai,:);
        ms   = adat.pos_solve_ms;

        if isempty(ms) || all(ms == 0)
            continue;
        end

        all_pos_ms = [all_pos_ms; ms(:)]; %#ok<AGROW>

        % --- Time-series ---
        ax = setup_dark_axes(fig1, n_agents, 3, (ai-1)*3 + 1);
        hold(ax,'on'); grid(ax,'on');
        ylabel(ax, aids{ai}, 'Color', col, 'FontWeight','bold', 'FontSize', 8);
        if ai == 1
            title(ax, 'Solve Duration (ms)', 'Color',[0.95 0.95 0.95],'FontSize',9);
        end
        sm = smooth_data(ms, opt.Smooth);
        plot(ax, adat.t_pos, sm, '-', 'Color', col, 'LineWidth', 1.2);
        p99 = prctile(ms, opt.WorstPct);
        yline(ax, mean(ms), '--', 'Color', [col, 0.7], 'LineWidth', 1.0);
        yline(ax, p99,      ':',  'Color', [1 0.5 0.5], 'LineWidth', 1.0);
        add_stats_text(ax, ms);

        % --- Histogram ---
        ax = setup_dark_axes(fig1, n_agents, 3, (ai-1)*3 + 2);
        hold(ax,'on'); grid(ax,'on');
        if ai == 1
            title(ax, 'Distribution (ms)', 'Color',[0.95 0.95 0.95],'FontSize',9);
        end
        histogram(ax, ms, opt.NBins, 'FaceColor', col, 'FaceAlpha', 0.7, ...
                  'EdgeColor', 'none');
        xline(ax, mean(ms), '--', 'Color', [1 1 1 0.8], 'LineWidth', 1.0);
        xline(ax, p99,      ':',  'Color', [1 0.5 0.5 0.9], 'LineWidth', 1.0);

        % --- CDF ---
        ax = setup_dark_axes(fig1, n_agents, 3, (ai-1)*3 + 3);
        hold(ax,'on'); grid(ax,'on');
        if ai == 1
            title(ax, 'CDF', 'Color',[0.95 0.95 0.95],'FontSize',9);
            ylabel(ax, 'Cumulative fraction', 'Color',[0.8 0.8 0.8],'FontSize',7);
        end
        ms_s = sort(ms);
        cdf  = (1:numel(ms_s))' / numel(ms_s);
        plot(ax, ms_s, cdf, '-', 'Color', col, 'LineWidth', 1.5);
        ylim(ax,[0,1]);
        xline(ax, p99, ':', 'Color', [1 0.5 0.5 0.9], 'LineWidth', 1.0);
        xlabel(ax, 'ms', 'Color',[0.8 0.8 0.8],'FontSize',7);
    end

    sgtitle(fig1, 'Position Solver  |  per agent', ...
            'Color',[0.95 0.95 0.95],'FontSize',11);
    maybe_save(fig1, opt.SaveDir, 'solver_position.png');

    % ============================================================
    %  FIG 2 — Assignment solver
    % ============================================================
    fig2 = make_figure('FlyChams — Assignment Solver Performance', opt.FigSize);

    ms_asgn = rec.fleet.assignment_solve_duration;
    t_asgn  = rec.fleet.t;

    if isempty(ms_asgn) || all(ms_asgn == 0)
        close(fig2);
    else
        % Time-series
        ax = setup_dark_axes(fig2, 1, 3, 1);
        hold(ax,'on'); grid(ax,'on');
        title(ax,'Solve Duration (ms)','Color',[0.95 0.95 0.95],'FontSize',9);
        xlabel(ax,'Time (s)','Color',[0.8 0.8 0.8],'FontSize',8);
        sm = smooth_data(ms_asgn, opt.Smooth);
        plot(ax, t_asgn, sm, '-', 'Color',[1.0 0.4 0.4],'LineWidth',1.3);
        p99 = prctile(ms_asgn, opt.WorstPct);
        yline(ax, mean(ms_asgn), '--','Color',[1 0.7 0.7 0.8],'LineWidth',1.0);
        yline(ax, p99,           ':','Color',[1 0.2 0.2 0.9],'LineWidth',1.0);
        add_stats_text(ax, ms_asgn);

        % Histogram
        ax = setup_dark_axes(fig2, 1, 3, 2);
        hold(ax,'on'); grid(ax,'on');
        title(ax,'Distribution (ms)','Color',[0.95 0.95 0.95],'FontSize',9);
        histogram(ax, ms_asgn, opt.NBins,'FaceColor',[1.0 0.4 0.4],'FaceAlpha',0.7,'EdgeColor','none');
        xline(ax, mean(ms_asgn),'--','Color',[1 1 1 0.8],'LineWidth',1.0);
        xline(ax, p99,          ':','Color',[1 0.2 0.2 0.9],'LineWidth',1.0);

        % Stats table (text box)
        ax = setup_dark_axes(fig2, 1, 3, 3);
        axis(ax,'off');
        title(ax,'Summary Statistics','Color',[0.95 0.95 0.95],'FontSize',9);
        stats_text = {
            sprintf('N samples     : %d',  numel(ms_asgn));
            sprintf('Mean          : %.3f ms', mean(ms_asgn));
            sprintf('Median        : %.3f ms', median(ms_asgn));
            sprintf('Std dev       : %.3f ms', std(ms_asgn));
            sprintf('Min           : %.3f ms', min(ms_asgn));
            sprintf('Max           : %.3f ms', max(ms_asgn));
            sprintf('P95           : %.3f ms', prctile(ms_asgn, 95));
            sprintf('P99           : %.3f ms', p99);
        };
        % Also append position solver pooled stats if available
        if ~isempty(all_pos_ms)
            stats_text{end+1} = '';
            stats_text{end+1} = '--- Position solver (pooled) ---';
            stats_text{end+1} = sprintf('Mean          : %.3f ms', mean(all_pos_ms));
            stats_text{end+1} = sprintf('P99           : %.3f ms', prctile(all_pos_ms,99));
            stats_text{end+1} = sprintf('Max           : %.3f ms', max(all_pos_ms));
        end
        text(ax, 0.05, 0.95, stats_text, 'Units','normalized', ...
             'VerticalAlignment','top','Color',[0.85 0.85 0.85],'FontSize',9, ...
             'FontName','Monospaced');

        sgtitle(fig2,'Assignment Solver','Color',[0.95 0.95 0.95],'FontSize',11);
        maybe_save(fig2, opt.SaveDir, 'solver_assignment.png');
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

function add_stats_text(ax, data)
    if isempty(data), return; end
    mu = mean(data); md = median(data); mx = max(data);
    txt = sprintf('μ=%.2f  med=%.2f  max=%.2f', mu, md, mx);
    xl = xlim(ax); yl = ylim(ax);
    text(ax, xl(1)+0.02*(xl(2)-xl(1)), yl(2)-0.04*(yl(2)-yl(1)), txt, ...
         'Color',[0.75 0.75 0.75],'FontSize',6,'VerticalAlignment','top');
end

function s = smooth_data(x, hw)
    if hw <= 0 || numel(x) < 3, s = x; return; end
    N = numel(x); s = zeros(N,1);
    for i=1:N
        lo = max(1,i-hw); hi = min(N,i+hw);
        s(i) = mean(x(lo:hi));
    end
end

function maybe_save(fig, save_dir, fname)
    if isempty(save_dir), return; end
    if ~exist(save_dir, 'dir'), mkdir(save_dir); end
    exportgraphics(fig, fullfile(save_dir, fname), 'Resolution', 150);
    fprintf('Saved: %s\n', fullfile(save_dir, fname));
end