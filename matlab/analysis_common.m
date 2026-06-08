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
        case 'figures_dir'
            varargout{1} = ensure_output_dir('figures');
        case 'stats_dir'
            varargout{1} = ensure_output_dir('stats');
        case 'export_figure'
            export_paper_figure(varargin{:});
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
        0.00 0.85 1.00
        0.20 0.80 0.40
        1.00 0.78 0.00
        0.55 0.20 1.00
        1.00 0.65 0.40
        0.00 0.55 1.00
        0.00 0.80 0.65
        0.85 0.00 0.85
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

function col = lighten_color(col, amount)
    col = col + amount * (1.0 - col);
end