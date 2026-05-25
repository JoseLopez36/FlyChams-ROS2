% ============================================================
% run_analysis.m
%
% Master analysis entry point for FlyChams MCAP recordings.
%
% Loads a recording, runs the full suite of analysis scripts, and
% optionally saves all figures to a timestamped output directory.
%
% USAGE
%   run_analysis                          % opens a file picker
%   run_analysis('../recordings/my_run/my_run_0.mcap')
%   run_analysis('../recordings/my_run/my_run_0.mcap', 'SaveFigs', true)
%
% PARAMETERS (name-value)
%   'SaveFigs'      false   Save PNG of every figure to output_dir
%   'OutputDir'     ''      Override the auto-generated output directory
%                           (default: matlab/figures/<recording_stem>_<timestamp>/)
%   'Verbose'       true    Print progress to console
%   'Decimate'      10      Spatial downsampling for trajectory plots
%   'Smooth'        5       Time-series smoothing half-window (samples)
%   'ViewWidth'     640     Tracking view width  (px)
%   'ViewHeight'    360     Tracking view height (px)
%   'CentralW'      1280    Central view width   (px)
%   'CentralH'      720     Central view height  (px)
%
% SCRIPTS CALLED
%   load_recording   — deserialise MCAP into structured rec struct
%   plot_trajectories — 2D/3D spatial paths, distance-to-goal
%   plot_metrics      — time-series dashboard (agent/target/cluster/mission)
%   plot_tracking     — image-plane quality (IoU, crop, OOB, zoom)
%   plot_solver       — solver timing distributions
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-05-24
% ============================================================

function run_analysis(mcap_path, varargin)

    % --------------------------------------------------------
    % Parse options
    % --------------------------------------------------------
    p = inputParser;
    if nargin >= 1
        addRequired(p, 'mcap_path', @ischar);
    end
    addParameter(p, 'SaveFigs',   false,  @islogical);
    addParameter(p, 'OutputDir',  '',     @ischar);
    addParameter(p, 'Verbose',    true,   @islogical);
    addParameter(p, 'Decimate',   10,     @isnumeric);
    addParameter(p, 'Smooth',     5,      @isnumeric);
    addParameter(p, 'ViewWidth',  640,    @isnumeric);
    addParameter(p, 'ViewHeight', 360,    @isnumeric);
    addParameter(p, 'CentralW',   1280,   @isnumeric);
    addParameter(p, 'CentralH',   720,    @isnumeric);
    if nargin >= 1
        parse(p, mcap_path, varargin{:});
    else
        parse(p, varargin{:});
    end
    opt = p.Results;

    % --------------------------------------------------------
    % File selection
    % --------------------------------------------------------
    if nargin < 1 || isempty(opt.mcap_path)
        [fname, fpath] = uigetfile({'*.mcap','MCAP files (*.mcap)'}, ...
                                    'Select FlyChams recording');
        if isequal(fname, 0)
            disp('Analysis cancelled.');
            return;
        end
        mcap_path = fullfile(fpath, fname);
    else
        mcap_path = opt.mcap_path;
    end

    if ~exist(mcap_path, 'file')
        error('run_analysis: file not found: %s', mcap_path);
    end

    % --------------------------------------------------------
    % Output directory
    % --------------------------------------------------------
    save_dir = '';
    if opt.SaveFigs
        if isempty(opt.OutputDir)
            [~, stem, ~] = fileparts(mcap_path);
            ts = datestr(now, 'yyyymmdd_HHMMSS');
            save_dir = fullfile(fileparts(mfilename('fullpath')), ...
                                'figures', [stem '_' ts]);
        else
            save_dir = opt.OutputDir;
        end
        if ~exist(save_dir, 'dir')
            mkdir(save_dir);
        end
        if opt.Verbose
            fprintf('[run_analysis] Figures will be saved to:\n  %s\n', save_dir);
        end
    end

    % --------------------------------------------------------
    % Step 1 — Load recording
    % --------------------------------------------------------
    if opt.Verbose, fprintf('\n=== Step 1 / 5  Loading recording ===\n'); end
    rec = load_recording(mcap_path, 'Verbose', opt.Verbose);

    if opt.Verbose
        fprintf('  Agents  : %d  (%s)\n', rec.meta.n_agents,   strjoin(rec.meta.agent_ids,   ', '));
        fprintf('  Targets : %d  (%s)\n', rec.meta.n_targets,  strjoin(rec.meta.target_ids,  ', '));
        fprintf('  Clusters: %d  (%s)\n', rec.meta.n_clusters, strjoin(rec.meta.cluster_ids, ', '));
        fprintf('  Duration: %.1f s\n',   rec.meta.duration_s);
    end

    % --------------------------------------------------------
    % Step 2 — Trajectories
    % --------------------------------------------------------
    if opt.Verbose, fprintf('\n=== Step 2 / 5  Trajectories ===\n'); end
    plot_trajectories(rec, ...
        'SaveDir',       save_dir, ...
        'Decimate',      opt.Decimate, ...
        'ShowSetpoints', true, ...
        'ShowClusters',  true);

    % --------------------------------------------------------
    % Step 3 — Metrics dashboard
    % --------------------------------------------------------
    if opt.Verbose, fprintf('\n=== Step 3 / 5  Metrics dashboard ===\n'); end
    plot_metrics(rec, ...
        'SaveDir', save_dir, ...
        'Smooth',  opt.Smooth);

    % --------------------------------------------------------
    % Step 4 — Tracking quality
    % --------------------------------------------------------
    if opt.Verbose, fprintf('\n=== Step 4 / 5  Tracking quality ===\n'); end
    plot_tracking(rec, ...
        'SaveDir',    save_dir, ...
        'ViewWidth',  opt.ViewWidth,  ...
        'ViewHeight', opt.ViewHeight, ...
        'CentralW',   opt.CentralW,   ...
        'CentralH',   opt.CentralH,   ...
        'Smooth',     opt.Smooth);

    % --------------------------------------------------------
    % Step 5 — Solver performance
    % --------------------------------------------------------
    if opt.Verbose, fprintf('\n=== Step 5 / 5  Solver performance ===\n'); end
    plot_solver(rec, ...
        'SaveDir', save_dir, ...
        'Smooth',  opt.Smooth);

    % --------------------------------------------------------
    % Summary
    % --------------------------------------------------------
    if opt.Verbose
        fprintf('\n=== Analysis complete ===\n');
        if opt.SaveFigs
            fprintf('Figures saved to: %s\n', save_dir);
        end
    end
end