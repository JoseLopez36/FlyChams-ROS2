% ============================================================
% generate_trajectories.m
%
% Target trajectory generation script for FlyChams-ROS2.
%
% Generates multi-target trajectory sets compatible with the
% flychams_simulation TrajectoryParser (tab-separated CSV, no header,
% columns: time  x  y  z, dt = 0.05 s).
%
% OUTPUT LAYOUT
%   <output_root>/<config_name>/trajectory_0.csv
%                trajectory_1.csv
%                ...
%                trajectory_N-1.csv
%
% HOW TO USE
%   1. Edit the USER PARAMETERS section below.
%   2. Run the script in MATLAB.
%   3. Copy or symlink the output folder into
%      src/flychams_common/config/trajectories/.
%   4. Point the relevant Excel configuration sheet to the new folder name.
%
% MOTION PRIMITIVES AVAILABLE
%   'lissajous'   - Lissajous figure (smooth, bounded)
%   'spiral'      - Inward/outward spiral
%   'random_walk' - Band-limited random walk (smooth stochastic)
%   'figure8'     - Figure-of-eight
%   'circle'      - Constant-radius circle
%   'waypoint'    - Piecewise linear interpolation through waypoints
%
% CLUSTER SUPPORT
%   Targets are organised into logical clusters.  Each cluster has an
%   independent centre and radius so that intra-cluster dispersion can
%   be tuned independently of inter-cluster separation.
%
% Author: Jose Francisco Lopez Ruiz
% Date:   2025-05-24
% ============================================================

clear; clc; close all;

% ============================================================
%  USER PARAMETERS
% ============================================================

% --- Output --------------------------------------------------
OUTPUT_ROOT = '../src/flychams_common/config/trajectories';
CONFIGURATION_NAME = 'Complex';

% --- Time axis -----------------------------------------------
T_TOTAL   = 1000.0;  % Total trajectory duration (s)
DT        = 0.05;    % Sample period (s) — must match TrajectoryParser expectation

% --- Cluster setup selector ----------------------------------
% Choose a named cluster layout defined in get_cluster_setup() below.
%
% Available setups:
%   'default'             — 4 clusters, 4 targets each (default demo)
%   'complex'             — 6 clusters for 3 agents × 2 tracking units; mixed dispersion and overlapping geometry
%
CLUSTER_SETUP = 'complex';

[CLUSTERS, CONFIGURATION_NAME] = get_cluster_setup(CLUSTER_SETUP, CONFIGURATION_NAME);

% --- Per-target dispersion -----------------------------------
% Each target inside a cluster is placed at a random offset within
% the cluster radius.  DISPERSION_FACTOR in [0,1] scales that offset:
%   0.0 → all targets start at the cluster centre (no dispersion)
%   1.0 → targets may start anywhere within the full cluster radius
DISPERSION_FACTOR = 0.6;

% --- Motion parameters (applied per cluster) -----------------
% All motion prims use these as defaults; override per-cluster below.
MP = struct();

% Lissajous
MP.lissajous.A      = 1.0;   % Amplitude multiplier (relative to cluster radius)
MP.lissajous.B      = 1.0;
MP.lissajous.a      = 3;     % X-frequency ratio
MP.lissajous.b      = 2;     % Y-frequency ratio
MP.lissajous.delta  = pi/4;  % Phase offset (rad)
MP.lissajous.T      = 120;   % Period of the slowest axis (s)

% Spiral
MP.spiral.r_max     = 1.0;   % Max radius multiplier (relative to cluster radius)
MP.spiral.r_min     = 0.1;   % Min radius multiplier
MP.spiral.omega     = 0.02;  % Angular velocity (rad/s)
MP.spiral.T_radial  = 200;   % Radial oscillation period (s)

% Random walk
MP.random_walk.speed       = 0.5;  % Mean speed (m/s)
MP.random_walk.turn_rate   = 0.3;  % Max turn rate (rad/s)
MP.random_walk.boundary    = 1.0;  % Boundary multiplier (relative to cluster radius)
MP.random_walk.smooth_win  = 21;   % Smoothing window length (samples, odd)

% Figure-8
MP.figure8.A   = 1.0;   % Semi-axis X multiplier
MP.figure8.B   = 0.6;   % Semi-axis Y multiplier
MP.figure8.T   = 90;    % Period (s)

% Circle
MP.circle.r    = 0.85;  % Radius multiplier
MP.circle.T    = 80;    % Period (s)

% Waypoint (default — each target gets random waypoints within cluster)
MP.waypoint.n_wp     = 8;    % Number of waypoints
MP.waypoint.v_cruise = 1.5;  % Cruise speed (m/s)

% --- 3-D motion ----------------------------------------------
% Set to true to generate non-zero Z profiles.
ENABLE_Z_MOTION = false;
MP.z.amplitude = 3.0;   % Vertical oscillation amplitude (m)
MP.z.period    = 60.0;  % Vertical oscillation period (s)

% --- Visualisation -------------------------------------------
VISUALISE       = true;   % Plot trajectories after generation
PLOT_CLUSTERS   = true;   % Overlay cluster enclosing circles
ANIMATE         = true;   % Animate playback (slow for many targets)
ANIM_STEP       = 10;     % Animation step (samples)

% --- Reproducibility -----------------------------------------
RNG_SEED = 42;

% ============================================================
%  INTERNAL LOGIC — do not edit below unless extending
% ============================================================

rng(RNG_SEED);

t = (0 : DT : T_TOTAL - DT)';
N_samples = length(t);

% Collect all trajectories
all_traj   = {};   % cell array of [N_samples x 3] matrices
target_idx = 0;

for c = 1 : size(CLUSTERS, 1)
    cx       = CLUSTERS(c, 1);
    cy       = CLUSTERS(c, 2);
    cz       = CLUSTERS(c, 3);
    radius   = CLUSTERS(c, 4);
    n_tgts   = CLUSTERS(c, 5);
    mtype    = CLUSTERS(c, 6);

    for k = 1 : n_tgts
        target_idx = target_idx + 1;

        % Unique random offset within cluster (dispersion)
        theta_off = 2*pi * rand();
        r_off     = radius * DISPERSION_FACTOR * rand();
        x0 = cx + r_off * cos(theta_off);
        y0 = cy + r_off * sin(theta_off);
        z0 = cz;

        % Phase offset per target for variety inside the cluster
        phase_k = 2*pi * (k - 1) / n_tgts;

        % Generate XY motion
        switch mtype
            case 1  % Lissajous
                xy = gen_lissajous(t, x0, y0, radius, MP.lissajous, phase_k);
            case 2  % Spiral
                xy = gen_spiral(t, x0, y0, radius, MP.spiral, phase_k);
            case 3  % Random walk
                xy = gen_random_walk(t, x0, y0, radius, MP.random_walk, RNG_SEED + target_idx);
            case 4  % Figure-8
                xy = gen_figure8(t, x0, y0, radius, MP.figure8, phase_k);
            case 5  % Circle
                xy = gen_circle(t, x0, y0, radius, MP.circle, phase_k);
            case 6  % Waypoint
                xy = gen_waypoint(t, x0, y0, radius, MP.waypoint, RNG_SEED + target_idx);
            otherwise
                error('Unknown motion type index %d for cluster %d', mtype, c);
        end

        % Generate Z motion
        if ENABLE_Z_MOTION
            z = z0 + MP.z.amplitude * sin(2*pi/MP.z.period * t + phase_k);
        else
            z = z0 * ones(N_samples, 1);
        end

        all_traj{target_idx} = [xy, z];
    end
end

N_targets = target_idx;

% ============================================================
%  WRITE CSV FILES
% ============================================================
out_dir = fullfile(OUTPUT_ROOT, CONFIGURATION_NAME);
if ~exist(out_dir, 'dir')
    mkdir(out_dir);
end

for i = 1 : N_targets
    fname = fullfile(out_dir, sprintf('trajectory_%d.csv', i - 1));
    traj  = all_traj{i};
    fid   = fopen(fname, 'w');
    if fid == -1
        error('Cannot open file for writing: %s', fname);
    end
    for s = 1 : N_samples
        fprintf(fid, '%.6f\t%.6f\t%.6f\t%.6f\t\n', t(s), traj(s,1), traj(s,2), traj(s,3));
    end
    fclose(fid);
    fprintf('Written: %s\n', fname);
end

fprintf('\nDone. %d trajectories written to:\n  %s\n', N_targets, out_dir);

% ============================================================
%  VISUALISATION
% ============================================================
if VISUALISE
    figure('Name', sprintf('FlyChams Trajectories — %s', CONFIGURATION_NAME), ...
           'NumberTitle', 'off', 'Color', [0.12 0.12 0.14]);
    ax = axes('Color', [0.15 0.15 0.18], 'XColor', [0.8 0.8 0.8], ...
              'YColor', [0.8 0.8 0.8], 'ZColor', [0.8 0.8 0.8], ...
              'GridColor', [0.4 0.4 0.4], 'GridAlpha', 0.4);
    hold on; grid on; axis equal;
    xlabel('X (m)', 'Color', [0.9 0.9 0.9]);
    ylabel('Y (m)', 'Color', [0.9 0.9 0.9]);
    if ENABLE_Z_MOTION
        zlabel('Z (m)', 'Color', [0.9 0.9 0.9]);
    end
    title(sprintf('%s  |  %d targets  |  T=%.0f s  |  dt=%.3f s', ...
          CONFIGURATION_NAME, N_targets, T_TOTAL, DT), ...
          'Color', [0.95 0.95 0.95], 'FontSize', 11);

    cmap = lines(N_targets);
    cluster_offset = 0;

    for c = 1 : size(CLUSTERS, 1)
        cx     = CLUSTERS(c, 1);
        cy     = CLUSTERS(c, 2);
        radius = CLUSTERS(c, 4);
        n_tgts = CLUSTERS(c, 5);

        % Draw cluster enclosing circle
        if PLOT_CLUSTERS
            ang = linspace(0, 2*pi, 180);
            plot(ax, cx + radius*cos(ang), cy + radius*sin(ang), '--', ...
                 'Color', [0.6 0.6 0.6], 'LineWidth', 0.8);
            text(ax, cx, cy, sprintf('C%d', c), 'Color', [0.6 0.6 0.6], ...
                 'HorizontalAlignment', 'center', 'FontSize', 8);
        end

        % Draw trajectories for this cluster (static view only)
        if ~ANIMATE
            for k = 1 : n_tgts
                idx  = cluster_offset + k;
                traj = all_traj{idx};
                col  = cmap(idx, :);

                if ENABLE_Z_MOTION
                    plot3(ax, traj(:,1), traj(:,2), traj(:,3), ...
                          'Color', [col, 0.55], 'LineWidth', 1.0);
                    plot3(ax, traj(1,1), traj(1,2), traj(1,3), 'o', ...
                          'Color', col, 'MarkerFaceColor', col, 'MarkerSize', 6);
                else
                    plot(ax, traj(:,1), traj(:,2), ...
                         'Color', [col, 0.55], 'LineWidth', 1.0);
                    plot(ax, traj(1,1), traj(1,2), 'o', ...
                         'Color', col, 'MarkerFaceColor', col, 'MarkerSize', 6);
                end

                % Label target
                text(ax, traj(1,1)+0.5, traj(1,2)+0.5, sprintf('T%d', idx), ...
                     'Color', col, 'FontSize', 7);
            end
        end

        cluster_offset = cluster_offset + n_tgts;
    end

    if ANIMATE
        h_pts = gobjects(N_targets, 1);
        for i = 1 : N_targets
            traj = all_traj{i};
            h_pts(i) = plot(ax, traj(1,1), traj(1,2), 's', ...
                            'Color', cmap(i,:), 'MarkerFaceColor', cmap(i,:), ...
                            'MarkerSize', 8);
        end
        for s = 1 : ANIM_STEP : N_samples
            for i = 1 : N_targets
                traj = all_traj{i};
                set(h_pts(i), 'XData', traj(s,1), 'YData', traj(s,2));
            end
            title(ax, sprintf('%s  |  t = %.1f s', CONFIGURATION_NAME, t(s)), ...
                  'Color', [0.95 0.95 0.95], 'FontSize', 11);

            drawnow;
            pause(0.05);
        end
    end
end

% ============================================================
%  CLUSTER SETUP HELPER
% ============================================================

function [clusters, config_name] = get_cluster_setup(name, config_name)
    % Returns the CLUSTERS matrix for the requested named setup.
    % If config_name is already non-default ('test'), it is preserved;
    % otherwise it is replaced with the setup name.

    % Column layout:
    %   [centre_x, centre_y, centre_z, radius, n_targets, motion_type_index]
    %
    % motion_type_index:
    %   1 = lissajous | 2 = spiral | 3 = random_walk
    %   4 = figure8   | 5 = circle | 6 = waypoint

    switch name

        % --------------------------------------------------------
        case 'default'
        % --------------------------------------------------------
        % Original demonstration setup — 4 clusters of 4 targets.
        % Well-separated, no intentional overlap.
        %
        %  Cluster 1 — Lissajous   (cx= 50, cy= 30, r=15, 4 tgts)
        %  Cluster 2 — Figure-8    (cx=-30, cy= 60, r=12, 4 tgts)
        %  Cluster 3 — Random walk (cx= 20, cy=-40, r=18, 4 tgts)
        %  Cluster 4 — Circle      (cx=-50, cy=-20, r=10, 4 tgts)

            clusters = [
            %  cx    cy   cz  radius  n_tgts  motion
               50,   30,   0,   15,     4,      1;   % C1 — Lissajous
              -30,   60,   0,   12,     4,      4;   % C2 — Figure-8
               20,  -40,   0,   18,     4,      3;   % C3 — Random walk
              -50,  -20,   0,   10,     4,      5;   % C4 — Circle
            ];
            if strcmp(config_name, 'test'), config_name = 'default'; end

        % --------------------------------------------------------
        case 'complex'
        % --------------------------------------------------------
        % Scenario: 3 agents, each with 2 tracking units → 6 clusters.
        %
        % Design intent:
        %   - Agent A (units A1, A2): one tight low-dispersion cluster
        %     (lissajous, r=7) + one medium cluster (random walk, r=14).
        %     A2 intentionally overlaps B1 to create one assignment
        %     ambiguity — the only deliberate interference in this setup.
        %
        %   - Agent B (units B1, B2): one medium circle cluster (r=12)
        %     that partially overlaps A2 + one compact figure-8 cluster
        %     (r=8) well-separated from all others.
        %
        %   - Agent C (units C1, C2): one tight spiral cluster (r=7)
        %     and one wide waypoint cluster (r=16), both clearly
        %     separated from A and B clusters.
        %
        % Overlap summary:
        %   A2 ↔ B1  : slight edge-graze (centres ~25 m apart, r=10+9)
        %              — the only intentional interference
        %
        %  Notation in comments: r=radius, d=tight(low) or L=loose(high)
        %                         n=n_targets per cluster

            clusters = [
            %  cx    cy   cz  radius  n_tgts  motion
            % --- Agent A ---
               10,   40,   0,    6,     2,      1;   % A1 tight  — Lissajous   d
              -20,   15,   0,   10,     3,      3;   % A2 medium — Random walk   (grazes B1)
            % --- Agent B ---
               10,    0,   0,    9,     3,      5;   % B1 medium — Circle        (grazes A2)
               45,   25,   0,    7,     2,      4;   % B2 compact— Figure-8
            % --- Agent C ---
              -45,  -15,   0,    6,     2,      2;   % C1 tight  — Spiral      d
               25,  -40,   0,   12,     4,      6;   % C2 wide   — Waypoint    L
            ];
            if strcmp(config_name, 'test'), config_name = 'complex'; end

        otherwise
            error('get_cluster_setup: unknown setup name ''%s''.', name);
    end
end

% ============================================================
%  MOTION PRIMITIVE FUNCTIONS
% ============================================================

function xy = gen_lissajous(t, x0, y0, radius, p, phase)
    % Lissajous figure centred at (x0,y0).
    % The trajectory periodically returns to the start, making it
    % suitable for the ping-pong replay in TargetState.
    omega = 2*pi / p.T;
    x = x0 + radius * p.A * sin(p.a * omega * t + p.delta + phase);
    y = y0 + radius * p.B * sin(p.b * omega * t       + phase);
    xy = [x, y];
end

function xy = gen_spiral(t, x0, y0, radius, p, phase)
    % Sinusoidally-modulated radius + constant angular velocity.
    % Stays within [r_min, r_max]*radius and reverses naturally.
    omega  = p.omega;
    r = radius * (p.r_min + (p.r_max - p.r_min) * ...
        0.5 .* (1 + sin(2*pi/p.T_radial * t)));
    x = x0 + r .* cos(omega * t + phase);
    y = y0 + r .* sin(omega * t + phase);
    xy = [x, y];
end

function xy = gen_random_walk(t, x0, y0, radius, p, seed)
    % Smooth stochastic trajectory with soft boundary repulsion.
    rng(seed);
    N  = length(t);
    dt = t(2) - t(1);

    theta = zeros(N, 1);
    theta(1) = 2*pi * rand();
    px = zeros(N, 1); px(1) = x0;
    py = zeros(N, 1); py(1) = y0;

    boundary = radius * p.boundary;

    for s = 2 : N
        % Boundary repulsion: steer back toward centre if near edge
        dx = x0 - px(s-1);
        dy = y0 - py(s-1);
        dist = sqrt(dx^2 + dy^2);

        if dist > 0.8 * boundary
            theta_to_centre = atan2(dy, dx);
            blend = min(1.0, (dist - 0.8*boundary) / (0.2*boundary));
            target_theta = theta_to_centre;
            dtheta = blend * angle_diff(target_theta, theta(s-1)) + ...
                     (1 - blend) * p.turn_rate * dt * (2*rand()-1);
        else
            dtheta = p.turn_rate * dt * (2*rand()-1);
        end

        theta(s) = theta(s-1) + dtheta;
        px(s) = px(s-1) + p.speed * dt * cos(theta(s));
        py(s) = py(s-1) + p.speed * dt * sin(theta(s));
    end

    % Smooth with a moving average to remove high-frequency jitter
    hw = floor(p.smooth_win / 2);
    px = smooth_signal(px, hw);
    py = smooth_signal(py, hw);

    xy = [px, py];
end

function xy = gen_figure8(t, x0, y0, radius, p, phase)
    % Lemniscate-style figure-of-eight (Bernoulli lemniscate parametrisation).
    omega = 2*pi / p.T;
    denom = 1 + sin(omega*t + phase).^2;
    x = x0 + radius * p.A * cos(omega*t + phase) ./ denom;
    y = y0 + radius * p.B * sin(omega*t + phase) .* cos(omega*t + phase) ./ denom;
    xy = [x, y];
end

function xy = gen_circle(t, x0, y0, radius, p, phase)
    % Constant-speed circle.
    omega = 2*pi / p.T;
    x = x0 + radius * p.r * cos(omega * t + phase);
    y = y0 + radius * p.r * sin(omega * t + phase);
    xy = [x, y];
end

function xy = gen_waypoint(t, x0, y0, radius, p, seed)
    % Piecewise linear path through random waypoints inside the cluster,
    % interpolated at constant cruise speed, then looped.
    rng(seed);
    N  = length(t);
    dt = t(2) - t(1);

    % Generate random waypoints within the cluster radius
    angles = 2*pi * rand(p.n_wp, 1);
    radii  = radius * sqrt(rand(p.n_wp, 1));  % sqrt for uniform disk sampling
    wpts_x = x0 + radii .* cos(angles);
    wpts_y = y0 + radii .* sin(angles);

    % Compute cumulative arc-length time for waypoints
    wp_x = [x0; wpts_x; x0];  % close the loop
    wp_y = [y0; wpts_y; y0];
    seg_len = sqrt(diff(wp_x).^2 + diff(wp_y).^2);
    seg_time = seg_len / p.v_cruise;
    wp_t = [0; cumsum(seg_time)];
    loop_T = wp_t(end);

    % Interpolate onto the uniform time axis (with looping)
    t_mod = mod(t, loop_T);
    px = interp1(wp_t, wp_x, t_mod, 'linear');
    py = interp1(wp_t, wp_y, t_mod, 'linear');

    xy = [px, py];
end

% ============================================================
%  UTILITY FUNCTIONS
% ============================================================

function d = angle_diff(a, b)
    % Signed shortest angular difference a - b in [-pi, pi]
    d = mod(a - b + pi, 2*pi) - pi;
end

function s = smooth_signal(x, hw)
    % Simple symmetric moving-average with edge handling.
    N = length(x);
    s = zeros(N, 1);
    for i = 1 : N
        lo = max(1, i - hw);
        hi = min(N, i + hw);
        s(i) = mean(x(lo:hi));
    end
end