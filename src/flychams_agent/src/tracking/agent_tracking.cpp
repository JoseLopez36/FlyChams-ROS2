#include "flychams_agent/tracking/agent_tracking.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void AgentTracking::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "tracking_rate", 20.0f);

        // Initialize data
        agent_ = Agent();
        solvers_.clear();

        // Get tracking parameters
        tracking_params_ = settings_tools_->getTrackingParameters(agent_id_);

        // Get relevant transform frames
        world_frame_ = transform_tools_->getGlobalFrame();
        n_frames_ = 0;
        for (const auto& unit : tracking_params_.observation_units_params)
        {
            if (unit.type == ObservationType::Camera)
            {
                optical_frames_.push_back(transform_tools_->getCameraOpticalFrame(agent_id_, unit.id));
                n_frames_++;
            }
        }

        // Initialize observation setpoints message
        agent_.observation_setpoints.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
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
        }

        // Initialize GUI setpoints message
        agent_.gui_setpoints.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        // Set full crop for all units
        CropMsg full_crop;
        full_crop.x = 0;
        full_crop.y = 0;
        full_crop.w = 0;
        full_crop.h = 0;
        full_crop.is_out_of_bounds = false;
        for (const auto& unit : tracking_params_.observation_units_params)
        {
            agent_.gui_setpoints.crops.push_back(full_crop);

            // Fill camera IDs based on unit type (Camera: unit ID, Window: central camera ID)
            if (unit.type == ObservationType::Camera)
            {
                agent_.gui_setpoints.camera_ids.push_back(unit.id);
            }
            else if (unit.type == ObservationType::Window)
            {
                agent_.gui_setpoints.camera_ids.push_back(tracking_params_.observation_units_params[0].id);
            }
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
        agent_.status_sub = topic_tools_->createAgentStatusSubscriber(agent_id_,
            std::bind(&AgentTracking::statusCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.clusters_sub = topic_tools_->createAgentClustersSubscriber(agent_id_,
            std::bind(&AgentTracking::clustersCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Create publisher for tracking and GUI setpoints
        agent_.observation_setpoints_pub = topic_tools_->createAgentObservationSetpointsPublisher(agent_id_);
        agent_.gui_setpoints_pub = topic_tools_->createAgentGuiSetpointsPublisher(agent_id_);

        // Set update timer
        update_timer_ = rclcpp::create_timer(node_,
            node_->get_clock(),
            std::chrono::duration<float>(1.0f / update_rate_),
            std::bind(&AgentTracking::update, this),
            module_cb_group_);
    }

    void AgentTracking::onShutdown()
    {
        // Destroy agent data
        agent_.status_sub.reset();
        agent_.clusters_sub.reset();
        agent_.observation_setpoints_pub.reset();
        agent_.gui_setpoints_pub.reset();
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
        // Check if we have a valid agent status and cluster assignments
        if (!agent_.has_status || !agent_.has_clusters)
        {
            RCLCPP_WARN(node_->get_logger(), "Agent tracking: Agent %s has no status or clusters", agent_id_.c_str());
            return; // Skip tracking if we don't have a valid agent status or clusters
        }

        // Check if we are in the correct state to track
        if (agent_.status != AgentStatus::MISSION)
        {
            RCLCPP_WARN(node_->get_logger(), "Agent tracking: Agent %s is not in the correct state to track",
                agent_id_.c_str());
            return;
        }

        // Convert clusters message to Eigen types
        int n = static_cast<int>(agent_.clusters.centers.size());
        Matrix3Xr tab_P = Matrix3Xr::Zero(3, n);
        RowVectorXr tab_r = RowVectorXr::Zero(n);
        for (size_t i = 0; i < n; i++)
        {
            tab_P.col(i) = RosUtils::fromMsg(agent_.clusters.centers[i]);
            tab_r(i) = agent_.clusters.radii[i];
        }

        // Get observation unit transforms (only for Camera type)
        std::vector<Matrix4r> tab_T(n_frames_);
        for (int c = 0; c < n_frames_; c++)
        {
            const TransformMsg& T = transform_tools_->getTransform(world_frame_, optical_frames_[c]);
            tab_T[c] = RosUtils::fromMsg(T);
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
            if (unit.type == ObservationType::Camera && unit.role == ObservationRole::Tracking)
            {
                std::tie(zoom_factor, rotation) = updateCamera(tab_P.col(i), tab_r(i), tab_T[i], solvers_[i]);
            }
            else if (unit.type == ObservationType::Camera && unit.role == ObservationRole::Central)
            {
                // Set central camera reference focal length
                zoom_factor = tracking_params_.observation_units_params[0].upsilon_ref;
                // Get central camera initial orientation
                rotation = settings_tools_->getMultiCamera(agent_id_, unit.id)->orientation;
            }
            else if (unit.type == ObservationType::Window)
            {
                std::tie(zoom_factor, crop) = updateWindow(tab_P.col(i), tab_r(i), tab_T[0], solvers_[i]);
            }

            // Update observation and GUI setpoints
            agent_.observation_setpoints.zoom_factors[i] = zoom_factor;
            if (unit.type == ObservationType::Camera)
            {
                RosUtils::toMsg(rotation, agent_.observation_setpoints.rotations[i]);
            }
            else if (unit.type == ObservationType::Window)
            {
                RosUtils::toMsg(crop, agent_.gui_setpoints.crops[i]);
            }

            i++;
        }

        // Publish tracking and GUI setpoints messages
        agent_.observation_setpoints_pub->publish(agent_.observation_setpoints);
        agent_.gui_setpoints_pub->publish(agent_.gui_setpoints);
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

} // namespace flychams::agent