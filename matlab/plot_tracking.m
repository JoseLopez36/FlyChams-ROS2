% ============================================================
% plot_tracking.m
%
% Image-plane tracking quality analysis for FlyChams recordings.
%
% All metrics are derived from ObservationSetpoints messages
% (crop ROI + zoom factors per observation unit) without requiring
% the compressed images to be decoded.
%
% Metrics computed per tracking unit:
%   - Crop area fraction   : (w*h) / (view_w*view_h)  ∈ [0,1]
%   - Crop centring error  : Euclidean offset of crop centre from
%                            image centre, normalised by image diagonal
%   - Out-of-bounds (OOB)  : fraction of frames where crop.is_out_of_bounds
%   - Zoom factor          : from ObservationSetpoints.zoom_factors
%   - Approximate IoU      : overlap of the crop ROI with the image frame
%                            (IoU of window vs. full image, so = crop_area_frac
%                            when fully in-bounds, < that when partially OOB)
%
% Produces three figures:
%   Fig 1 — Per-agent / per-unit heatmap dashboard
%   Fig 2 — Time-series panels (crop fraction, centring, zoom) per agent
%   Fig 3 — OOB rate bar chart + IoU CDF across all tracking units
%
% USAGE
%   plot_tracking(rec)
%   plot_tracking(rec, 'SaveDir', '/path/to/figs', 'ViewWidth', 640, 'ViewHeight', 360)
%
% INPUT
%   rec  – struct returned by load_recording()
%
% PARAMETERS (name-value)
%   'SaveDir'     ''          Save PNGs here if non-empty
%   'ViewWidth'   640         Tracking view width  (px) — must match stream config
%   'ViewHeight'  360         Tracking view height (px)
%   'CentralW'    1280        Central view width   (px)
%   'CentralH'    720         Central view height  (px)
%   'Smooth'      10          Smoothing half-window (samples)
%   'FigSize'     [1400 800]
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-05-24
% ============================================================

function plot_tracking(rec, varargin)

    p = inputParser;
    addRequired(p,  'rec');
    addParameter(p, 'SaveDir',   '',        @ischar);
    addParameter(p, 'ViewWidth',  640,      @isnumeric);
    addParameter(p, 'ViewHeight', 360,      @isnumeric);
    addParameter(p, 'CentralW',  1280,      @isnumeric);
    addParameter(p, 'CentralH',   720,      @isnumeric);
    addParameter(p, 'Smooth',     10,       @isnumeric);
    addParameter(p, 'FigSize',  [1400 800], @isnumeric);
    parse(p, rec, varargin{:});
    opt = p.Results;

    tw = opt.ViewWidth;  th = opt.ViewHeight;
    cw = opt.CentralW;   ch = opt.CentralH;
    img_diag_t = sqrt(tw^2 + th^2);
    img_diag_c = sqrt(cw^2 + ch^2);

    aids = fieldnames(rec.observations);
    n_agents = numel(aids);
    if n_agents == 0
        warning('plot_tracking: no observation data found in recording.');
        return;
    end

    agent_cmap = lines(n_agents);

    % --------------------------------------------------------
    % Pre-compute per-agent, per-unit metrics
    % --------------------------------------------------------
    % obs_stats.(AGENTID)(unit_idx):
    %   .crop_frac    Nx1   area fraction
    %   .centre_err   Nx1   normalised centring error
    %   .iou          Nx1   approximate IoU vs. full image
    %   .oob          Nx1   logical out-of-bounds
    %   .zoom         Nx1   zoom factor
    %   .t            Nx1   timestamps
    %   .label        string
    %   .role         0=central 1=tracking (inferred from view dims)

    stats = struct();
    for ai = 1:n_agents
        obs = rec.observations.(aids{ai});
        if isempty(obs.t), continue; end
        N  = numel(obs.t);
        K  = size(obs.crops.x, 2);
        entry = struct();
        for k = 1:K
            cx_k = obs.crops.x(:,k) + obs.crops.w(:,k)/2;
            cy_k = obs.crops.y(:,k) + obs.crops.h(:,k)/2;
            w_k  = obs.crops.w(:,k);
            h_k  = obs.crops.h(:,k);
            oob_k = obs.crops.oob(:,k);

            % Determine view dimensions by checking crop magnitude
            % Units with large crops are more likely to be the central view.
            % (Heuristic: if median area > 50% of central view, it's central)
            med_area = median(w_k .* h_k);
            if med_area > 0.5 * cw * ch
                vw = cw; vh = ch; diag = img_diag_c;
            else
                vw = tw; vh = th; diag = img_diag_t;
            end

            % Crop area fraction
            area_frac = (w_k .* h_k) / (vw * vh);
            area_frac = min(area_frac, 1.0);

            % Centring error (normalised by image diagonal)
            centre_x = vw / 2;
            centre_y = vh / 2;
            centre_err = sqrt((cx_k - centre_x).^2 + (cy_k - centre_y).^2) / diag;

            % Approximate IoU: intersection of crop with full image / union
            % When fully in bounds: IoU = crop_area / image_area  (crop ⊂ image)
            % When OOB: IoU is reduced proportionally
            ix1 = max(obs.crops.x(:,k), 0);
            iy1 = max(obs.crops.y(:,k), 0);
            ix2 = min(obs.crops.x(:,k) + w_k, vw);
            iy2 = min(obs.crops.y(:,k) + h_k, vh);
            inter_area = max(ix2 - ix1, 0) .* max(iy2 - iy1, 0);
            union_area = (w_k .* h_k) + (vw * vh) - inter_area;
            iou = inter_area ./ max(union_area, 1);

            zoom_k = obs.zoom_factors(:,k);

            entry(k).t           = obs.t;
            entry(k).crop_frac   = area_frac;
            entry(k).centre_err  = centre_err;
            entry(k).iou         = iou;
            entry(k).oob         = oob_k;
            entry(k).zoom        = zoom_k;
            entry(k).label       = sprintf('unit%d', k);
        end
        stats.(aids{ai}) = entry;
    end

    % ============================================================
    %  FIG 1 — Summary heatmap dashboard
    % ============================================================
    fig1 = make_figure('FlyChams — Tracking Quality Dashboard', opt.FigSize);

    metrics_names = {'Mean Crop Frac', 'Mean Centre Err', 'OOB Rate', 'Mean IoU', 'Mean Zoom'};
    n_metrics = numel(metrics_names);

    % Build matrix: rows = agents, cols = metrics
    % (averaged across all units per agent)
    heat_mat = nan(n_agents, n_metrics);
    for ai = 1:n_agents
        if ~isfield(stats, aids{ai}), continue; end
        entry = stats.(aids{ai});
        K = numel(entry);
        if K == 0, continue; end
        cf=0; ce=0; ob=0; io=0; zm=0;
        for k=1:K
            cf = cf + mean(entry(k).crop_frac);
            ce = ce + mean(entry(k).centre_err);
            ob = ob + mean(double(entry(k).oob));
            io = io + mean(entry(k).iou);
            zm = zm + mean(entry(k).zoom);
        end
        heat_mat(ai,:) = [cf, ce, ob, io, zm] / K;
    end

    ax1 = setup_dark_axes(fig1, 1, 1, 1);
    imagesc(ax1, heat_mat);
    colormap(ax1, 'parula');
    cb = colorbar(ax1);
    cb.Color = [0.8 0.8 0.8];
    set(ax1, 'XTick', 1:n_metrics, 'XTickLabel', metrics_names, ...
             'XTickLabelRotation', 20, ...
             'YTick', 1:n_agents, 'YTickLabel', aids, ...
             'FontSize', 8);
    title(ax1, 'Tracking Quality Heatmap (agent × metric, time-averaged)', ...
          'Color', [0.95 0.95 0.95], 'FontSize', 10);

    % Annotate cells
    for ai = 1:n_agents
        for mi = 1:n_metrics
            if ~isnan(heat_mat(ai,mi))
                text(ax1, mi, ai, sprintf('%.3f', heat_mat(ai,mi)), ...
                     'HorizontalAlignment','center','VerticalAlignment','middle', ...
                     'Color','w','FontSize',8,'FontWeight','bold');
            end
        end
    end

    maybe_save(fig1, opt.SaveDir, 'tracking_dashboard.png');

    % ============================================================
    %  FIG 2 — Time-series per agent (tracking units only)
    % ============================================================
    fig2 = make_figure('FlyChams — Tracking Time Series', opt.FigSize);
    n_cols = 3;

    for ai = 1:n_agents
        if ~isfield(stats, aids{ai}), continue; end
        entry = stats.(aids{ai});
        K = numel(entry);
        if K == 0, continue; end
        col = agent_cmap(ai,:);

        % Crop fraction
        ax = setup_dark_axes(fig2, n_agents, n_cols, (ai-1)*n_cols + 1);
        hold(ax,'on'); grid(ax,'on');
        ylabel(ax, aids{ai}, 'Color', col, 'FontWeight','bold','FontSize',8);
        if ai==1, title(ax,'Crop Area Fraction','Color',[0.95 0.95 0.95],'FontSize',9); end
        ylim(ax,[0,1]);
        for k=1:K
            e = entry(k);
            sm = smooth_data(e.crop_frac, opt.Smooth);
            plot(ax, e.t, sm, '-', 'Color', darken(col, 0.25*(k-1)), 'LineWidth',1.1);
        end

        % Centring error
        ax = setup_dark_axes(fig2, n_agents, n_cols, (ai-1)*n_cols + 2);
        hold(ax,'on'); grid(ax,'on');
        if ai==1, title(ax,'Centring Error (norm.)','Color',[0.95 0.95 0.95],'FontSize',9); end
        for k=1:K
            e = entry(k);
            sm = smooth_data(e.centre_err, opt.Smooth);
            plot(ax, e.t, sm, '-', 'Color', darken(col, 0.25*(k-1)), 'LineWidth',1.1);
        end

        % Zoom factor
        ax = setup_dark_axes(fig2, n_agents, n_cols, (ai-1)*n_cols + 3);
        hold(ax,'on'); grid(ax,'on');
        if ai==1, title(ax,'Zoom Factor','Color',[0.95 0.95 0.95],'FontSize',9); end
        for k=1:K
            e = entry(k);
            sm = smooth_data(e.zoom, opt.Smooth);
            plot(ax, e.t, sm, '-', 'Color', darken(col, 0.25*(k-1)), 'LineWidth',1.1);
        end
    end
    sgtitle(fig2,'Per-Agent Tracking Time Series','Color',[0.95 0.95 0.95],'FontSize',11);
    maybe_save(fig2, opt.SaveDir, 'tracking_timeseries.png');

    % ============================================================
    %  FIG 3 — OOB rate + IoU CDF
    % ============================================================
    fig3 = make_figure('FlyChams — OOB & IoU Summary', opt.FigSize);

    % Left: OOB rate per agent (mean across units)
    ax3a = setup_dark_axes(fig3, 1, 2, 1);
    hold(ax3a,'on'); grid(ax3a,'on');
    title(ax3a,'Out-of-Bounds Rate per Agent','Color',[0.95 0.95 0.95],'FontSize',10);
    ylabel(ax3a,'OOB Rate (0–1)','Color',[0.8 0.8 0.8],'FontSize',9);
    oob_rates = zeros(1, n_agents);
    for ai=1:n_agents
        if ~isfield(stats, aids{ai}), continue; end
        entry = stats.(aids{ai});
        K = numel(entry);
        if K==0, continue; end
        rate = 0;
        for k=1:K, rate = rate + mean(double(entry(k).oob)); end
        oob_rates(ai) = rate / K;
    end
    b = bar(ax3a, oob_rates, 'FaceColor','flat');
    for ai=1:n_agents, b.CData(ai,:) = agent_cmap(ai,:); end
    set(ax3a,'XTick',1:n_agents,'XTickLabel',aids,'XTickLabelRotation',25,'FontSize',8);
    ylim(ax3a,[0,1]);

    % Right: IoU CDF (all tracking units pooled)
    ax3b = setup_dark_axes(fig3, 1, 2, 2);
    hold(ax3b,'on'); grid(ax3b,'on');
    title(ax3b,'IoU CDF (all tracking units)','Color',[0.95 0.95 0.95],'FontSize',10);
    xlabel(ax3b,'IoU','Color',[0.8 0.8 0.8],'FontSize',9);
    ylabel(ax3b,'Cumulative Fraction','Color',[0.8 0.8 0.8],'FontSize',9);
    xlim(ax3b,[0,1]); ylim(ax3b,[0,1]);

    for ai=1:n_agents
        if ~isfield(stats, aids{ai}), continue; end
        entry = stats.(aids{ai});
        col = agent_cmap(ai,:);
        iou_all = [];
        for k=1:numel(entry)
            iou_all = [iou_all; entry(k).iou(:)]; %#ok<AGROW>
        end
        if isempty(iou_all), continue; end
        iou_sorted = sort(iou_all);
        cdf = (1:numel(iou_sorted))' / numel(iou_sorted);
        plot(ax3b, iou_sorted, cdf, '-', 'Color', col, 'LineWidth',1.5, 'DisplayName', aids{ai});
    end
    legend(ax3b,'TextColor',[0.85 0.85 0.85],'Color',[0.18 0.18 0.20],'EdgeColor',[0.4 0.4 0.4],'FontSize',8);

    maybe_save(fig3, opt.SaveDir, 'tracking_oob_iou.png');
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

function s = smooth_data(x, hw)
    if hw <= 0 || numel(x) < 3, s = x; return; end
    N = numel(x); s = zeros(N,1);
    for i=1:N
        lo = max(1,i-hw); hi = min(N,i+hw);
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