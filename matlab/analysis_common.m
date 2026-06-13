function varargout = analysis_common(action, varargin)
% Shared helpers for paper-oriented bag analysis scripts.

    switch action
        case 'style'
            varargout{1} = paper_style();
        case 'palette'
            varargout{1} = agent_palette();
        case 'axis'
            varargout{1} = paper_ax(varargin{:});
        case 'bag_label'
            varargout{1} = bag_label(varargin{:});
        case 'bag_timestamps'
            varargout{1} = bag_timestamps(varargin{:});
        case 'trim_mask'
            varargout{1} = trim_mask(varargin{:});
        case 'trim_limits'
            [varargout{1}, varargout{2}] = trim_limits(varargin{:});
        case 'values_in_window'
            varargout{1} = values_in_window(varargin{:});
        case 'crop_series'
            [varargout{1}, varargout{2}] = crop_series(varargin{:});
        case 'apply_trim_xlim'
            apply_trim_xlim(varargin{:});
        case 'padded_xlim'
            padded_xlim(varargin{:});
        case 'padded_ylim'
            padded_ylim(varargin{:});
        case 'plot_min_mean_max'
            plot_min_mean_max(varargin{:});
        case 'min_mean_max_lines'
            varargout{1} = min_mean_max_lines(varargin{:});
        case 'paper_figure'
            varargout{1} = paper_figure(varargin{:});
        case 'figures_dir'
            varargout{1} = ensure_output_dir('figures');
        case 'stats_dir'
            varargout{1} = ensure_output_dir('stats');
        case 'export_figure'
            export_paper_figure(varargin{:});
        case 'export_report_figure'
            export_report_figure(varargin{:});
        case 'export_stats'
            export_stats(varargin{:});
        case 'stats_lines'
            varargout{1} = stats_lines(varargin{:});
        case 'print_stats'
            print_stats(varargin{:});
        case 'lighten_color'
            varargout{1} = lighten_color(varargin{:});
        otherwise
            error('analysis_common:UnknownAction', 'Unknown action "%s".', action);
    end
end

% ============================================================
%  STYLE
% ============================================================

function style = paper_style()
    style.single_width = 3.45;
    style.double_width = 7.15;
    style.short_height = 2.65;
    style.tall_height = 4.20;
    style.line_width = 1.4;
    style.axis_width = 1.0;
    style.axis_font_size = 12;
    style.text_font_size = 12;
    style.marker_size = 5;
    style.blue = 1/255 * [0, 113, 188];
    style.orange = 1/255 * [216, 82, 24];
    style.green = 1/255 * [0, 176, 80];
    style.purple = [0.55, 0.20, 1.00];
    style.red = [0.80, 0.10, 0.10];
end

function colors = agent_palette()
    colors = [
        1/255 * [0, 113, 188]
        1/255 * [216, 82, 24]
        1/255 * [0, 176, 80]
    ];
end

function ax = paper_ax(fig, rows, cols, idx)
    style = paper_style();
    ax = subplot(rows, cols, idx, 'Parent', fig);
    set(ax, 'Color',     [1.0  1.0  1.0 ], ...
            'XColor',    [0.10 0.10 0.10], ...
            'YColor',    [0.10 0.10 0.10], ...
            'GridColor', [0.75 0.75 0.75], ...
            'GridAlpha', 0.45, ...
            'LineWidth', style.axis_width, ...
            'FontSize',  style.axis_font_size, ...
            'TickLabelInterpreter', 'latex', ...
            'Box', 'on');
end

function fig = paper_figure(style, width, height)
    fig = figure('NumberTitle', 'off', 'Color', [1 1 1], ...
                 'Units', 'inches', 'Position', [0, 0, width, height]);
    set(fig, 'DefaultAxesFontSize', style.axis_font_size);
end

% ============================================================
%  TIME WINDOW
% ============================================================

function t = bag_timestamps(sel)
    tlist = sel.MessageList.Time;
    t     = seconds(seconds(tlist - tlist(1)));
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

function values = values_in_window(values, t, t_start, t_end)
    values = values(trim_mask(t, t_start, t_end));
end

function [vals, t] = crop_series(vals, t_raw, t_start, t_end, zero_origin, warn_id)
    if nargin < 5 || isempty(zero_origin), zero_origin = true; end
    if nargin < 6, warn_id = ''; end

    mask = trim_mask(t_raw, t_start, t_end);
    if ~any(mask)
        if ~isempty(warn_id)
            warning('%s: crop [%.1f, %.1f] s yields no samples (bag range: [%.1f, %.1f] s)', ...
                warn_id, t_start, t_end, t_raw(1), t_raw(end));
        end
        vals = [];
        t    = [];
        return;
    end
    vals = vals(mask);
    t    = t_raw(mask);
    if zero_origin
        t = t - t(1);
    end
end

function apply_trim_xlim(ax, t, t_start, t_end)
    [lo, hi] = trim_limits(t, t_start, t_end);
    xlim(ax, [lo, hi]);
end

% ============================================================
%  PLOT HELPERS
% ============================================================

function plot_min_mean_max(ax, values, color)
    values = values(:);
    values = values(~isnan(values));
    if isempty(values)
        return;
    end
    yline(ax, min(values),  '--', 'Color', color, 'LineWidth', 1.0);
    yline(ax, mean(values), '--', 'Color', color, 'LineWidth', 1.0);
    yline(ax, max(values),  '--', 'Color', color, 'LineWidth', 1.0);
end

function padded_xlim(ax, values, pad_fraction)
    if nargin < 3, pad_fraction = 0.10; end
    padded_lim(ax, values, 'x', pad_fraction);
end

function padded_ylim(ax, values, pad_fraction)
    if nargin < 3, pad_fraction = 0.10; end
    padded_lim(ax, values, 'y', pad_fraction);
end

function padded_lim(ax, values, axis_name, pad_fraction)
    if nargin < 4, pad_fraction = 0.10; end
    values = values(:);
    values = values(~isnan(values));
    if isempty(values)
        return;
    end
    lo = min(values);
    hi = max(values);
    span = hi - lo;
    if span > 0
        pad = pad_fraction * span;
    else
        pad = max(1.0, abs(lo) * 0.002);
    end
    lim = [lo - pad, hi + pad];
    if strcmp(axis_name, 'x')
        xlim(ax, lim);
    else
        ylim(ax, lim);
    end
end

% ============================================================
%  STATS
% ============================================================

function lines = min_mean_max_lines(values, unit)
    values = values(:);
    values = values(~isnan(values));
    if nargin < 2, unit = ''; end
    if ~isempty(unit), u = [' ' unit]; else, u = ''; end
    if isempty(values)
        lines = {
            '  Min  : n/a';
            '  Mean : n/a';
            '  Max  : n/a';
        };
        return;
    end
    lines = {
        sprintf('  Min  : %.3f%s', min(values),  u);
        sprintf('  Mean : %.3f%s', mean(values), u);
        sprintf('  Max  : %.3f%s', max(values),  u);
    };
end

function lines = stats_lines(data, unit)
    data = data(:);
    data = data(~isnan(data));
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
    lines = stats_lines(data, unit);
    fprintf('  %s\n', strjoin(lines, sprintf('\n  ')));
end

% ============================================================
%  I/O
% ============================================================

function out_dir = ensure_output_dir(name)
    name = char(name);
    if is_absolute_path(name)
        out_dir = name;
    else
        out_dir = fullfile(pwd, name);
    end
    if ~exist(out_dir, 'dir')
        mkdir(out_dir);
    end
end

function label = bag_label(bag_path)
    bag_path = char(bag_path);
    [~, label] = fileparts(bag_path);
end

function tf = is_absolute_path(path)
    tf = startsWith(path, filesep) || ~isempty(regexp(path, '^[A-Za-z]:[\\/]', 'once'));
end

function export_paper_figure(fig, out_dir, label, name)
    bag_dir = ensure_output_dir(fullfile(out_dir, label));
    set(fig, 'PaperPositionMode', 'auto');
    drawnow;
    exportgraphics(fig, fullfile(bag_dir, sprintf('%s.png', name)), 'Resolution', 300);
end

function export_report_figure(fig, out_dir, name)
    ensure_output_dir(out_dir);
    set(fig, 'PaperPositionMode', 'auto');
    drawnow;
    exportgraphics(fig, fullfile(out_dir, sprintf('%s.png', name)), 'Resolution', 300);
end

function export_stats(out_dir, label, name, lines)
    bag_dir = ensure_output_dir(fullfile(out_dir, label));
    file_path = fullfile(bag_dir, sprintf('%s.txt', name));
    fid = fopen(file_path, 'w');
    if fid < 0
        error('analysis_common:StatsOpenFailed', 'Could not open "%s" for writing.', file_path);
    end
    cleanup = onCleanup(@() fclose(fid));
    fprintf(fid, '%s\n', strjoin(lines, newline));
end

function col = lighten_color(col, amount)
    col = col + amount * (1.0 - col);
end