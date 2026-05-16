#include "flychams_agent/tracking/agent_tracking.hpp"

using namespace flychams::common;

using namespace flychams::agent;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void AgentTracking::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate
    update_rate_ = node_->getParameterOr<float>("tracking_rate", 20.0f);

    // Initialize data
    agent_ = Agent();
    solvers_.clear();

    // Get tracking parameters
    tracking_params_ = node_->getSettings()->getTrackingParameters(agent_id_);

    // Get relevant transform frames
    world_frame_ = node_->getGlobalFrame();
    n_frames_ = 0;
    for (const auto& unit : tracking_params_.observation_units_params)
    {
        if (unit.type == ObservationType::Camera)
        {
            optical_frames_.push_back(node_->getCameraOpticalFrame(agent_id_, unit.id));
            n_frames_++;
        }
    }

    // Initialize observation setpoints message
    agent_.observation_setpoints.header = node_->createHeader(node_->getGlobalFrame());
    agent_.observation_setpoints.n_o = tracking_params_.n_o;
    agent_.observation_setpoints.n_t = tracking_params_.n_t;
    agent_.observation_setpoints.n_c = tracking_params_.n_c;
    agent_.observation_setpoints.n_w = tracking_params_.n_w;
    for (const auto& unit : tracking_params_.observation_units_params)
    {
        agent_.observation_setpoints.ids.push_back(unit.id);
        agent_.observation_setpoints.types.push_back(static_cast<uint8_t>(unit.type));
        agent_.observation_setpoints.roles.push_back(static_cast<uint8_t>(unit.role));
        agent_.observation_setpoints.zoom_factors.push_back(unit.upsilon_ref);
        agent_.observation_setpoints.rotations.push_back(Vector3Msg());
        agent_.observation_setpoints.crops.push_back(CropMsg());
    }

    // Create observation unit solvers
    for (const auto& unit : tracking_params_.observation_units_params)
    {
        // Create instance using unit parameters
        ObservationSolver::SharedPtr solver = std::make_shared<ObservationSolver>(unit);

        // Initialize solver
        solver->reset();

        // Add to solvers
        solvers_.push_back(solver);
    }

    // Create subscribers for agent status, position and clusters
    agent_.status_sub = node_->createAgentStatusSubscriber(agent_id_,
        std::bind(&AgentTracking::statusCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());
    agent_.clusters_sub = node_->createAgentClustersSubscriber(agent_id_,
        std::bind(&AgentTracking::clustersCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Create publisher for observation setpoints
    agent_.observation_setpoints_pub = node_->createObservationSetpointsPublisher(agent_id_);
    
    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentTracking::update, this));
}

void AgentTracking::onModuleShutdown()
{
    // Destroy agent data
    agent_.status_sub.reset();
    agent_.clusters_sub.reset();
    agent_.observation_setpoints_pub.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void AgentTracking::statusCallback(const AgentStatusMsg::SharedPtr msg)
{
    // Update agent status
    agent_.status = static_cast<AgentStatus>(msg->status);
    agent_.has_status = true;
}

void AgentTracking::clustersCallback(const AgentClustersMsg::SharedPtr msg)
{
    // Update agent clusters
    agent_.clusters = *msg;
    agent_.has_clusters = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update tracking
// ════════════════════════════════════════════════════════════════════════════

void AgentTracking::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Agent tracking: Skipping update due to invalid status");
        return;
    }

    // Convert clusters message to Eigen types
    size_t n = agent_.clusters.centers.size();
    Matrix3Xr tab_P = Matrix3Xr::Zero(3, n);
    RowVectorXr tab_r = RowVectorXr::Zero(n);
    for (size_t i = 0; i < n; i++)
    {
        tab_P.col(i) = node_->fromMsg(agent_.clusters.centers[i]);
        tab_r(i) = agent_.clusters.radii[i];
    }

    // Get observation unit transforms (only for Camera type)
    std::vector<Matrix4r> tab_T(n_frames_);
    for (int c = 0; c < n_frames_; c++)
    {
        const TransformMsg& T = node_->getTransform(world_frame_, optical_frames_[c]);
        tab_T[c] = node_->fromMsg(T);
    }

    // Solve tracking for each observation unit
    int i = 0;
    for (const auto& unit : tracking_params_.observation_units_params)
    {
        // Initialize variables
        float zoom_factor;
        Vector3r rotation; // Only for Camera type
        Crop crop;         // Only for Window type

        // Solve based on unit type
        if (unit.type == ObservationType::Camera && unit.role == ObservationRole::Central)
        {
            // Set central camera reference focal length
            zoom_factor = tracking_params_.observation_units_params[0].upsilon_ref;
            // Get central camera initial orientation
            rotation = node_->getSettings()->getMultiCamera(agent_id_, unit.id)->orientation;
        }
        else if (unit.type == ObservationType::Camera && unit.role == ObservationRole::Tracking)
        {
            std::tie(zoom_factor, rotation) = updateCamera(tab_P.col(i), tab_r(i), tab_T[i], solvers_[i]);
        }
        else if (unit.type == ObservationType::Window)
        {
            std::tie(zoom_factor, crop) = updateWindow(tab_P.col(i), tab_r(i), tab_T[0], solvers_[i]);
        }

        // Update observation setpoints
        agent_.observation_setpoints.zoom_factors[i] = zoom_factor;
        if (unit.type == ObservationType::Camera)
        {
            node_->toMsg(rotation, agent_.observation_setpoints.rotations[i]);
        }
        else if (unit.type == ObservationType::Window)
        {
            node_->toMsg(crop, agent_.observation_setpoints.crops[i]);
        }

        i++;
    }

    // Publish observation setpoints
    agent_.observation_setpoints_pub->publish(agent_.observation_setpoints);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool AgentTracking::checkStatus()
{
    // Check 1: Mission must be active
    if (!node_->isMissionActive())
    {
        RCLCPP_WARN(node_->get_logger(), "Agent tracking: Mission is not active");
        return false;
    }

    // Check 2: Agent must have a valid status
    if (!agent_.has_status)
    {
        RCLCPP_WARN(node_->get_logger(), "Agent tracking: Agent %s has no status", agent_id_.c_str());
        return false;
    }

    // Check 3: Agent must be in ACTIVE state
    if (agent_.status != AgentStatus::ACTIVE)
    {
        RCLCPP_WARN(node_->get_logger(), "Agent tracking: Agent %s is not in ACTIVE state", agent_id_.c_str());
        return false;
    }

    // Check 4: Agent must have cluster assignments
    if (!agent_.has_clusters)
    {
        RCLCPP_WARN(node_->get_logger(), "Agent tracking: Agent %s has no clusters", agent_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// TRACKING: Tracking methods
// ════════════════════════════════════════════════════════════════════════════

std::tuple<float, Vector3r> AgentTracking::updateCamera(const Vector3r& P, const float& r, const Matrix4r& T, ObservationSolver::SharedPtr solver)
{
    // Create auxiliary transform
    // This transform is used in the calculation of tracking orientation
    // Reference: world frame
    // Location: optical frame origin
    // Rotation: 180 deg around X
    Matrix4r wTaux = Matrix4r::Identity();
    // Location
    wTaux.block<3, 1>(0, 3) = T.block<3, 1>(0, 3);
    // Rotation
    const Quaternionr wQaux = Quaternionr(0.0f, 1.0f, 0.0f, 0.0f);  // w, x, y, z
    wTaux.block<3, 3>(0, 0) = MathUtils::quatToMatrix(wQaux);

    // Compute camera setpoint
    const auto& [focal, auxRPYc] = solver->runCamera(P, r, wTaux);

    // Convert auxiliary orientation to world frame (same X, inverted Y and Z)
    const Vector3r wRPYc = Vector3r(auxRPYc(0), auxRPYc(1) - M_PI_2, auxRPYc(2) - M_PI_2);

    // Return focal length and orientation
    return std::make_tuple(focal, wRPYc);
}

std::tuple<float, Crop> AgentTracking::updateWindow(const Vector3r& P, const float& r, const Matrix4r& T, ObservationSolver::SharedPtr solver)
{
    // Compute window setpoint and return resolution factor and crop
    return solver->runWindow(P, r, T);
}