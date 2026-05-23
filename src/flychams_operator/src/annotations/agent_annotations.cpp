#include "flychams_operator/annotations/agent_annotations.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("annotation_rate", 10.0f);

    // Resolve agent colour index from mission settings
    agent_idx_ = node_->getSettings()->getAgent(agent_id_)->idx;
    // View dimensions
    central_view_width_   = node_->getParameterOr<int>("central_view.width", 854);
    central_view_height_  = node_->getParameterOr<int>("central_view.height", 480);
    tracking_view_width_  = node_->getParameterOr<int>("tracking_view.width", 427);
    tracking_view_height_ = node_->getParameterOr<int>("tracking_view.height", 240);

    // Create publishers from settings config
    const auto& tracking_config = node_->getSettings()->getTracking(agent_id_);
    for (const auto& [camera_id, camera] : tracking_config.multi_camera_set)
    {
        if (camera->role == ObservationRole::Central)
        {
            // Resolve original resolution for window crop overlay scaling
            original_view_width_  = camera->camera.resolution(0);
            original_view_height_ = camera->camera.resolution(1);

            // Central camera publisher
            annotation_pubs_[camera_id] = node_->createAnnotationsPublisher(agent_id_, camera_id);

            // One publisher per crop window associated with this central camera
            for (const auto& [window_id, _] : tracking_config.multi_window_set)
            {
                annotation_pubs_[window_id] = node_->createAnnotationsPublisher(agent_id_, window_id);
            }
        }
        else if (camera->role == ObservationRole::Tracking)
        {
            annotation_pubs_[camera_id] = node_->createAnnotationsPublisher(agent_id_, camera_id);
        }
    }

    // Subscriber: observation setpoints
    setpoints_sub_ = node_->createObservationSetpointsSubscriber(agent_id_,
        std::bind(&AgentAnnotations::observationSetpointsCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Subscriber: agent clusters
    clusters_sub_ = node_->createAgentClustersSubscriber(agent_id_,
        std::bind(&AgentAnnotations::clustersCallback, this, std::placeholders::_1),
        node_->getSubscriptionOptions());

    // Subscribers: camera_info per camera (for live intrinsics)
    for (const auto& [camera_id, camera] : tracking_config.multi_camera_set)
    {
        camera_info_subs_[camera_id] = node_->createCameraInfoSubscriber(agent_id_, camera_id,
            [this, camera_id](const CameraInfoMsg::SharedPtr msg) { cameraInfoCallback(camera_id, msg); },
            node_->getSubscriptionOptions());
    }

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentAnnotations::update, this));
}

void AgentAnnotations::onModuleShutdown()
{
    update_timer_.reset();
    annotation_pubs_.clear();
    setpoints_sub_.reset();
    clusters_sub_.reset();
    camera_info_subs_.clear();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::observationSetpointsCallback(const ObservationSetpointsMsg::SharedPtr msg)
{
    setpoints_ = msg;
    has_setpoints_ = true;
}

void AgentAnnotations::clustersCallback(const AgentClustersMsg::SharedPtr msg)
{
    if (msg->centers.empty())
        return;

    clusters_.clear();
    const size_t n = msg->centers.size();
    for (size_t i = 1; i < n; ++i)  // skip index 0 (global cluster)
    {
        ClusterData cluster;
        cluster.center  = msg->centers[i];
        cluster.radius  = msg->radii[i];
        cluster.unit_id = msg->unit_ids[i];
        clusters_.push_back(cluster);
    }
    has_clusters_ = true;
}

void AgentAnnotations::cameraInfoCallback(const ID& camera_id, const CameraInfoMsg::SharedPtr msg)
{
    Intrinsics& k = intrinsics_[camera_id];
    // K = [fx, 0, cx, 0, fy, cy, 0, 0, 1]  (row-major, 9 elements)
    if (msg->k.size() < 9)
        return;
    k.fx    = msg->k[0];
    k.fy    = msg->k[4];
    k.cx    = msg->k[2];
    k.cy    = msg->k[5];
    k.valid = (k.fx > 0.0 && k.fy > 0.0);
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::update()
{
    if (!has_setpoints_)
    {
        return;
    }

    const auto& sp = *setpoints_;
    const size_t n = sp.ids.size();

    for (size_t i = 0; i < n; ++i)
    {
        const ID& unit_id = sp.ids[i];
        if (annotation_pubs_.find(unit_id) == annotation_pubs_.end())
            continue;

        const uint8_t type = i < sp.types.size() ? sp.types[i] : 0;
        const uint8_t role = i < sp.roles.size() ? sp.roles[i] : 0;

        // Central camera → full resolution view, tracking units → half resolution
        const bool is_central = (role == 1 /* Central */);
        const int view_w = is_central ? central_view_width_  : tracking_view_width_;
        const int view_h = is_central ? central_view_height_ : tracking_view_height_;

        if (type == 1 /* Camera */)
        {
            publishCameraAnnotations(i, view_w, view_h);
        }
        else if (type == 2 /* Window */)
        {
            publishWindowAnnotations(i, view_w, view_h);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// ANNOTATION HELPERS
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::publishCameraAnnotations(size_t idx, int view_w, int view_h) const
{
    const auto& sp = *setpoints_;
    const auto stamp = node_->now();

    // Get zoom factor (focal)
    const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;

    FoxImageAnnotationsMsg msg;

    const float W  = static_cast<float>(view_w);
    const float H  = static_cast<float>(view_h);
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;
    const float side = std::min(W, H);

    // Agent color from palette
    const FoxColorMsg hud_color = AnnotationHelpers::makeColor(AgentColors::get(agent_idx_));
    const FoxColorMsg bg     = AnnotationHelpers::makeColor(CameraAnnotations::kBg);
    const FoxColorMsg white  = AnnotationHelpers::makeColor(Colors::kWhite);

    // ── Crosshair with centre gap ──────────────────────────────────────────
    {
        const float arm = side * CameraAnnotations::kCrosshairArmFrac;
        const float gap = side * CameraAnnotations::kCrosshairGapFrac;
        // Horizontal: left segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx - arm, cy), AnnotationHelpers::pt(cx - gap, cy), hud_color, CameraAnnotations::kCrosshairThick);
        // Horizontal: right segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx + gap, cy), AnnotationHelpers::pt(cx + arm, cy), hud_color, CameraAnnotations::kCrosshairThick);
        // Vertical: top segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy - arm), AnnotationHelpers::pt(cx, cy - gap), hud_color, CameraAnnotations::kCrosshairThick);
        // Vertical: bottom segment
        AnnotationHelpers::addLine(msg, stamp, AnnotationHelpers::pt(cx, cy + gap), AnnotationHelpers::pt(cx, cy + arm), hud_color, CameraAnnotations::kCrosshairThick);
    }

    // ── Centre dot ────────────────────────────────────────────────────────
    {
        FoxCircleAnnotationMsg dot;
        dot.timestamp = stamp;
        dot.position.x = cx;
        dot.position.y = cy;
        dot.diameter = CameraAnnotations::kCentreDotDiam;
        dot.fill_color = hud_color;
        dot.outline_color = white;
        dot.thickness = 1.0f;
        msg.circles.push_back(dot);
    }

    // ── Window crop overlays (central view only) ──────────────────────────
    const uint8_t role = idx < sp.roles.size() ? sp.roles[idx] : 0;
    const bool is_central = (role == 1 /* Central */);
    if (is_central && CameraAnnotations::kShowWindowsOnCentral)
    {
        const size_t n = sp.ids.size();
        for (size_t j = 0; j < n; ++j)
        {
            const uint8_t wtype = j < sp.types.size() ? sp.types[j] : 0;
            if (wtype != 2 /* Window */ || j >= sp.crops.size())
            {
                continue;
            }

            const auto& crop = sp.crops[j];
            const float sx  = static_cast<float>(view_w) / static_cast<float>(original_view_width_);
            const float sy  = static_cast<float>(view_h) / static_cast<float>(original_view_height_);
            const float wx0 = static_cast<float>(crop.x)           * sx;
            const float wy0 = static_cast<float>(crop.y)           * sy;
            const float wx1 = static_cast<float>(crop.x + crop.w)  * sx;
            const float wy1 = static_cast<float>(crop.y + crop.h)  * sy;

            // Window overlay: agent color normally, gray when OOB
            Color win_base = AgentColors::get(agent_idx_);
            if (crop.is_out_of_bounds)
            {
                win_base = Colors::kGray;
            }
            const FoxColorMsg win_color = AnnotationHelpers::makeColor(withAlpha(win_base, 0.80f));

            // Box
            {
                FoxPointsAnnotationMsg rect;
                rect.type = FoxPointsAnnotationMsg::LINE_LOOP;
                rect.timestamp = stamp;
                rect.points = {
                    AnnotationHelpers::pt(wx0, wy0), AnnotationHelpers::pt(wx1, wy0),
                    AnnotationHelpers::pt(wx1, wy1), AnnotationHelpers::pt(wx0, wy1)
                };
                rect.outline_color = win_color;
                rect.thickness = CameraAnnotations::kWinOverlayBoxThick;
                msg.points.push_back(rect);
            }

            // Unit ID label (top-left of crop box)
            {
                const std::string unit_id = j < sp.ids.size() ? sp.ids[j] : "?";
                FoxTextAnnotationMsg label;
                label.timestamp = stamp;
                label.position.x = wx0 + CameraAnnotations::kWinOverlayIdFontMarginX;
                label.position.y = wy0 + CameraAnnotations::kWinOverlayIdFontMarginY;
                label.text = unit_id;
                label.font_size = CameraAnnotations::kWinOverlayIdFontSz;
                label.text_color = win_color;
                label.background_color = AnnotationHelpers::makeColor(CameraAnnotations::kBg);
                msg.texts.push_back(label);
            }
        }
    }

    // ── Role / unit ID badge ───────────────────────────────────
    if (CameraAnnotations::kShowBadge)
    {
        const std::string unit_id = idx < sp.ids.size() ? sp.ids[idx] : "?";
        const uint8_t role = idx < sp.roles.size() ? sp.roles[idx] : 0;
        const std::string role_str = (role == 1) ? "CENTRAL" : "TRACKING";

        FoxTextAnnotationMsg badge;
        badge.timestamp = stamp;
        badge.position.x = CameraAnnotations::kBadgeMarginX;
        badge.position.y = CameraAnnotations::kBadgeMarginY;
        badge.text = agent_id_ + " - " + role_str + " - " + unit_id;
        badge.font_size = CameraAnnotations::kBadgeFontSize;
        badge.text_color = hud_color;
        badge.background_color = bg;
        msg.texts.push_back(badge);
    }

    // ── HUD text ────────────────────────
    if (CameraAnnotations::kShowHud)
    {
        std::ostringstream oss;
        oss << std::fixed;
        oss << "z=" << std::setprecision(2) << zoom;
        if (idx < sp.rotations.size())
        {
            constexpr float kR2D = 180.0f / static_cast<float>(M_PI);
            const float pitch_deg = sp.rotations[idx].y * kR2D;
            const float yaw_deg   = sp.rotations[idx].z * kR2D;
            oss << "  p=" << std::setprecision(1) << pitch_deg << "\xC2\xB0"
                << "  y=" << yaw_deg << "\xC2\xB0";
        }

        FoxTextAnnotationMsg hud;
        hud.timestamp = stamp;
        hud.position.x = CameraAnnotations::kHudMarginX;
        hud.position.y = H - CameraAnnotations::kHudMarginY;
        hud.text = oss.str();
        hud.font_size = CameraAnnotations::kHudFontSize;
        hud.text_color = hud_color;
        hud.background_color = bg;
        msg.texts.push_back(hud);
    }

    // ── Cluster overlays ──────────────────────────────────────────────────
    if (has_clusters_)
    {
        const ID& camera_id = sp.ids[idx];
        appendClusterOverlays(msg, stamp, camera_id, view_w, view_h, !is_central);
    }

    annotation_pubs_.at(sp.ids[idx])->publish(msg);
}

void AgentAnnotations::publishWindowAnnotations(size_t idx, int view_w, int view_h) const
{
    const auto& sp = *setpoints_;
    const auto stamp = node_->now();

    if (idx >= sp.crops.size())
    {
        return;
    }

    const auto& crop = sp.crops[idx];
    FoxImageAnnotationsMsg msg;

    const float y1   = static_cast<float>(view_h);
    const float zoom = idx < sp.zoom_factors.size() ? sp.zoom_factors[idx] : 1.0f;
    const bool  oob  = crop.is_out_of_bounds;

    // Agent color from palette (orange tint when out of bounds)
    Color agent_color = AgentColors::get(agent_idx_);
    if (oob)
    {
        agent_color = Colors::kOrange;
    }
    const FoxColorMsg text_color = AnnotationHelpers::makeColor(agent_color);
    const FoxColorMsg bg         = AnnotationHelpers::makeColor(WindowAnnotations::kBg);

    // ── Role / unit ID badge (top-left of the window view) ───────────────
    if (WindowAnnotations::kShowBadge)
    {
        const std::string unit_id = idx < sp.ids.size() ? sp.ids[idx] : "?";
        FoxTextAnnotationMsg badge;
        badge.timestamp = stamp;
        badge.position.x = WindowAnnotations::kBadgeMarginX;
        badge.position.y = WindowAnnotations::kBadgeMarginY;
        badge.text = "WINDOW - " + unit_id + (oob ? "  [OOB]" : "");
        badge.font_size = WindowAnnotations::kBadgeFontSize;
        badge.text_color = text_color;
        badge.background_color = bg;
        msg.texts.push_back(badge);
    }

    // ── HUD text (crop size + zoom, bottom-left corner)
    if (WindowAnnotations::kShowHud)
    {
        std::ostringstream oss;
        oss << crop.w << "x" << crop.h
            << "  z=" << std::fixed << std::setprecision(2) << zoom;

        FoxTextAnnotationMsg hud;
        hud.timestamp = stamp;
        hud.position.x = WindowAnnotations::kHudMarginX;
        hud.position.y = y1 - WindowAnnotations::kHudMarginY;
        hud.text = oss.str();
        hud.font_size = WindowAnnotations::kHudFontSize;
        hud.text_color = text_color;
        hud.background_color = bg;
        msg.texts.push_back(hud);
    }

    // ── Cluster overlays (projected into window crop space) ───────────────
    if (has_clusters_)
    {
        // Find the central camera id (role==1) from setpoints to get intrinsics
        ID central_camera_id;
        for (size_t j = 0; j < sp.ids.size(); ++j)
        {
            if (j < sp.roles.size() && sp.roles[j] == 1 /* Central */)
            {
                central_camera_id = sp.ids[j];
                break;
            }
        }
        if (!central_camera_id.empty())
        {
            appendClusterOverlaysWindow(msg, stamp, central_camera_id, idx, view_w, view_h, true);
        }
    }

    annotation_pubs_.at(sp.ids[idx])->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// CLUSTER OVERLAY HELPERS
// ════════════════════════════════════════════════════════════════════════════

void AgentAnnotations::appendClusterOverlays(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const ID& camera_id, int view_w, int view_h, bool only_show_assigned) const
{
    const auto it = intrinsics_.find(camera_id);
    if (it == intrinsics_.end() || !it->second.valid)
        return;

    const std::string optical_frame = node_->getCameraOpticalFrame(agent_id_, camera_id);
    const std::string world_frame   = node_->getGlobalFrame();

    // Build projection matrices
    const Matrix4r wTc = buildWTc(node_->getTransform(world_frame, optical_frame));
    const Matrix3r K   = buildK(it->second);

    // Scale: original camera resolution → display resolution
    const float sx  = static_cast<float>(view_w) / static_cast<float>(original_view_width_);
    const float sy  = static_cast<float>(view_h)  / static_cast<float>(original_view_height_);
    const float fvw = static_cast<float>(view_w);
    const float fvh = static_cast<float>(view_h);

    const FoxColorMsg dash_color = ClusterAnnotations::kDash;

    for (const auto& cluster : clusters_)
    {
        // Only show the cluster assigned to this camera unit
        if (only_show_assigned && cluster.unit_id != camera_id)
            continue;

        const Vector3r wP(
            static_cast<float>(cluster.center.x),
            static_cast<float>(cluster.center.y),
            static_cast<float>(cluster.center.z));

        // Project 3D sphere rim → true ellipse in image space (original resolution)
        std::vector<FoxPoint2Msg> rim = projectRim(wP, cluster.radius, wTc, K,
                                                    ClusterAnnotations::kRimPoints);
        if (rim.empty())
            continue;

        // Scale rim to display resolution and check bounds
        bool out_of_bounds = false;
        for (auto& p : rim)
        {
            p.x *= sx;
            p.y *= sy;
            if (p.x < 0.0f || p.x > fvw || p.y < 0.0f || p.y > fvh)
            {
                out_of_bounds = true;
                break;
            }
        }
        if (out_of_bounds)
            continue;

        AnnotationHelpers::addDashedPolyline(msg, stamp, rim, dash_color,
                                             ClusterAnnotations::kThickness,
                                             ClusterAnnotations::kNDashes,
                                             ClusterAnnotations::kDashFrac);
    }
}

void AgentAnnotations::appendClusterOverlaysWindow(FoxImageAnnotationsMsg& msg, const rclcpp::Time& stamp, const ID& camera_id, size_t sp_idx, int view_w, int view_h, bool only_show_assigned) const
{
    const auto it = intrinsics_.find(camera_id);
    if (it == intrinsics_.end() || !it->second.valid)
        return;

    const auto& sp = *setpoints_;
    if (sp_idx >= sp.crops.size())
        return;

    // Unit ID for this window — used to match the correct cluster
    const ID window_id = sp_idx < sp.ids.size() ? sp.ids[sp_idx] : ID{};

    const auto& crop = sp.crops[sp_idx];
    const std::string optical_frame = node_->getCameraOpticalFrame(agent_id_, camera_id);
    const std::string world_frame   = node_->getGlobalFrame();

    // Build projection matrices (same central camera)
    const Matrix4r wTc = buildWTc(node_->getTransform(world_frame, optical_frame));
    const Matrix3r K   = buildK(it->second);

    // Scale: original resolution → crop-local display pixels
    const float crop_sx = static_cast<float>(view_w) / static_cast<float>(crop.w > 0 ? crop.w : 1);
    const float crop_sy = static_cast<float>(view_h)  / static_cast<float>(crop.h > 0 ? crop.h : 1);
    const float fvw     = static_cast<float>(view_w);
    const float fvh     = static_cast<float>(view_h);

    const FoxColorMsg dash_color = ClusterAnnotations::kDash;

    for (const auto& cluster : clusters_)
    {
        // Only show the cluster assigned to this window unit
        if (only_show_assigned && cluster.unit_id != window_id)
            continue;

        const Vector3r wP(
            static_cast<float>(cluster.center.x),
            static_cast<float>(cluster.center.y),
            static_cast<float>(cluster.center.z));

        // Project 3D sphere rim → true ellipse in original image space
        std::vector<FoxPoint2Msg> rim = projectRim(wP, cluster.radius, wTc, K,
                                                    ClusterAnnotations::kRimPoints);
        if (rim.empty())
            continue;

        // Map rim from original image coords → crop-local display coords and check bounds
        bool out_of_bounds = false;
        for (auto& p : rim)
        {
            p.x = (p.x - static_cast<float>(crop.x)) * crop_sx;
            p.y = (p.y - static_cast<float>(crop.y)) * crop_sy;
            if (p.x < 0.0f || p.x > fvw || p.y < 0.0f || p.y > fvh)
            {
                out_of_bounds = true;
                break;
            }
        }
        if (out_of_bounds)
            continue;

        AnnotationHelpers::addDashedPolyline(msg, stamp, rim, dash_color,
                                             ClusterAnnotations::kThickness,
                                             ClusterAnnotations::kNDashes,
                                             ClusterAnnotations::kDashFrac);
    }
}

Matrix4r AgentAnnotations::buildWTc(const TransformMsg& tf)
{
    // Quaternion → rotation matrix (Eigen convention: w,x,y,z)
    const auto& q = tf.rotation;
    const auto& t = tf.translation;
    Eigen::Quaternionf quat(
        static_cast<float>(q.w),
        static_cast<float>(q.x),
        static_cast<float>(q.y),
        static_cast<float>(q.z));
    Matrix4r wTc = Matrix4r::Identity();
    wTc.block<3, 3>(0, 0) = quat.normalized().toRotationMatrix();
    wTc(0, 3) = static_cast<float>(t.x);
    wTc(1, 3) = static_cast<float>(t.y);
    wTc(2, 3) = static_cast<float>(t.z);
    return wTc;
}

Matrix3r AgentAnnotations::buildK(const AgentAnnotations::Intrinsics& intr)
{
    Matrix3r K = Matrix3r::Zero();
    K(0, 0) = static_cast<float>(intr.fx);
    K(1, 1) = static_cast<float>(intr.fy);
    K(0, 2) = static_cast<float>(intr.cx);
    K(1, 2) = static_cast<float>(intr.cy);
    K(2, 2) = 1.0f;
    return K;
}

std::vector<FoxPoint2Msg> AgentAnnotations::projectRim(const Vector3r& wP, float radius, const Matrix4r& wTc, const Matrix3r& K, int n_pts = 64)
{
    // View direction: camera Z axis in world = third column of wTc rotation
    const Vector3r view_dir = wTc.block<3, 1>(0, 2);

    // Build two orthogonal axes in the plane perpendicular to view_dir
    Vector3r u = view_dir.cross(Vector3r::UnitZ());
    if (u.norm() < 1e-4f)
        u = view_dir.cross(Vector3r::UnitX());
    u.normalize();
    const Vector3r v = view_dir.cross(u).normalized();

    std::vector<FoxPoint2Msg> rim;
    rim.reserve(n_pts);

    const float step = 2.0f * static_cast<float>(M_PI) / static_cast<float>(n_pts);
    for (int i = 0; i < n_pts; ++i)
    {
        const float a        = static_cast<float>(i) * step;
        const Vector3r wRim  = wP + radius * (std::cos(a) * u + std::sin(a) * v);
        const Vector2r px    = VisionUtils::projectPoint(wRim, wTc, K);
        FoxPoint2Msg p;
        p.x = px(0);
        p.y = px(1);
        rim.push_back(p);
    }
    return rim;
}