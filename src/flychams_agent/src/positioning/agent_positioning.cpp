#include "flychams_agent/positioning/agent_positioning.hpp"

using namespace flychams::core;

namespace flychams::agent
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void AgentPositioning::onInit()
    {
        // Get parameters from parameter server
        // Get update rate
        update_rate_ = RosUtils::getParameterOr<float>(node_, "positioning_rate", 1.0f);
        // Get solver parameters
        solver_mode_ = static_cast<PositionSolver::SolverMode>(RosUtils::getParameterOr<uint8_t>(node_, "positioning.solver_mode", 0));
        // Get generic solver parameters
        solver_params_.eps = RosUtils::getParameterOr<float>(node_, "positioning.eps", 1.0e-1f);
        solver_params_.tol = RosUtils::getParameterOr<float>(node_, "positioning.convergence_tolerance", 1.0e-5f);
        solver_params_.max_iter = RosUtils::getParameterOr<int>(node_, "positioning.max_iterations", 100);
        // Get PSO parameters
        solver_params_.num_particles = RosUtils::getParameterOr<int>(node_, "positioning.num_particles", 50);
        solver_params_.w_max = RosUtils::getParameterOr<float>(node_, "positioning.w_max", 0.4f);
        solver_params_.w_min = RosUtils::getParameterOr<float>(node_, "positioning.w_min", 0.1f);
        solver_params_.c1 = RosUtils::getParameterOr<float>(node_, "positioning.c1", 1.0f);
        solver_params_.c2 = RosUtils::getParameterOr<float>(node_, "positioning.c2", 1.0f);
        solver_params_.stagnation_limit = RosUtils::getParameterOr<int>(node_, "positioning.stagnation_limit", 5);
        // Get ALC-PSO parameters
        solver_params_.max_lifespan = RosUtils::getParameterOr<int>(node_, "positioning.max_lifespan", 60);
        solver_params_.num_challenger_tests = RosUtils::getParameterOr<int>(node_, "positioning.num_challenger_tests", 10);
        // Get Nesterov parameters
        solver_params_.lipschitz_constant = RosUtils::getParameterOr<float>(node_, "positioning.lipschitz_constant", 0.0f);

        // Initialize data
        agent_ = Agent();

        // Initialize setpoint message
        agent_.setpoint.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
        agent_.setpoint.point = PointMsg();

        // Get relevant transform frames
        world_frame_ = transform_tools_->getGlobalFrame();
        const core::TrackingParameters& tracking_params = settings_tools_->getTrackingParameters(agent_id_);
        central_optical_frame_ = transform_tools_->getCameraOpticalFrame(agent_id_, tracking_params.observation_units_params[0].id);

        // Create and initialize solver
        solver_ = createSolver(agent_id_, solver_params_, solver_mode_);

        // Create subscribers for agent status, position and clusters
        agent_.status_sub = topic_tools_->createAgentStatusSubscriber(agent_id_,
            std::bind(&AgentPositioning::statusCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.position_sub = topic_tools_->createAgentGlobalPositionSubscriber(agent_id_,
            std::bind(&AgentPositioning::positionCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);
        agent_.clusters_sub = topic_tools_->createAgentClustersSubscriber(agent_id_,
            std::bind(&AgentPositioning::clustersCallback, this, std::placeholders::_1), sub_options_with_module_cb_group_);

        // Create publisher for agent setpoint
        agent_.setpoint_pub = topic_tools_->createAgentPositionSetpointPublisher(agent_id_);

        // Set update timer
        update_timer_ = rclcpp::create_timer(node_, 
            node_->get_clock(), 
            std::chrono::duration<float>(1.0f / update_rate_), 
            std::bind(&AgentPositioning::update, this), 
            module_cb_group_);
    }

    void AgentPositioning::onShutdown()
    {
        // Destroy solver
        solver_->destroy();
        // Destroy agent data
        agent_.status_sub.reset();
        agent_.position_sub.reset();
        agent_.clusters_sub.reset();
        agent_.setpoint_pub.reset();
        // Destroy update timer
        update_timer_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS: Callback functions
    // ════════════════════════════════════════════════════════════════════════════

    void AgentPositioning::statusCallback(const AgentStatusMsg::SharedPtr msg)
    {
        // Update agent status
        agent_.status = static_cast<AgentStatus>(msg->status);
        agent_.has_status = true;
    }

    void AgentPositioning::positionCallback(const PointStampedMsg::SharedPtr msg)
    {
        // Update agent position
        agent_.position = msg->point;
        agent_.has_position = true;
    }

    void AgentPositioning::clustersCallback(const AgentClustersMsg::SharedPtr msg)
    {
        // Update agent clusters
        agent_.clusters = *msg;
        agent_.has_clusters = true;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE: Update positioning
    // ════════════════════════════════════════════════════════════════════════════

    void AgentPositioning::update()
    {
        // Check if we have a valid agent status, position and cluster assignments
        if (!agent_.has_status || !agent_.has_position || !agent_.has_clusters)
        {
            RCLCPP_WARN(node_->get_logger(), "Agent positioning: Agent %s has no status, position or clusters", agent_id_.c_str());
            return; // Skip positioning if we don't have a valid agent status, position or clusters
        }

        // Check if we are in the correct state to position
        if (agent_.status != AgentStatus::MISSION)
        {
            RCLCPP_WARN(node_->get_logger(), "Agent positioning: Agent %s is not in the correct state to position",
                agent_id_.c_str());
            return;
        }

        // Convert messages to Eigen types
        Vector3r x0 = RosUtils::fromMsg(agent_.position);
        int n = static_cast<int>(agent_.clusters.centers.size());
        Matrix3Xr tab_P = Matrix3Xr::Zero(3, n);
        RowVectorXr tab_r = RowVectorXr::Zero(n);
        for (int i = 0; i < n; i++)
        {
            tab_P.col(i) = RosUtils::fromMsg(agent_.clusters.centers[i]);
            tab_r(i) = agent_.clusters.radii[i];
        }

        // Get central observation unit transform
        const TransformMsg& wTcentral_msg = transform_tools_->getTransform(world_frame_, central_optical_frame_);
        Matrix4r wTcentral = RosUtils::fromMsg(wTcentral_msg);

        // Solve agent positioning
        float J;
        const auto& start = std::chrono::high_resolution_clock::now();
        Vector3r optimal_position = solver_->run(tab_P, tab_r, x0, wTcentral, J);
        const auto& end = std::chrono::high_resolution_clock::now();
        float time_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        RCLCPP_DEBUG(node_->get_logger(), "Agent positioning: Computed optimal position (J = %.2f): (xOpt = %.2f, %.2f, %.2f) in %.2f us",
            J, optimal_position(0), optimal_position(1), optimal_position(2), time_elapsed);

        // Publish position
        agent_.setpoint.header.stamp = RosUtils::now(node_);
        RosUtils::toMsg(optimal_position, agent_.setpoint.point);
        agent_.setpoint_pub->publish(agent_.setpoint);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // POSITIONING: Positioning methods
    // ════════════════════════════════════════════════════════════════════════════

    PositionSolver::SharedPtr AgentPositioning::createSolver(const std::string& agent_id, const PositionSolver::Parameters& solver_params, const PositionSolver::SolverMode& solver_mode)
    {
        // Create solver instance
        PositionSolver::SharedPtr solver = std::make_shared<PositionSolver>();

        // Get config
        const auto& config_ptr = settings_tools_->getConfig();
        const auto& agent_ptr = settings_tools_->getAgent(agent_id);
        const auto& tracking_params = settings_tools_->getTrackingParameters(agent_id);

        // Get cost parameters for each tracking unit
        CostFunctions::CostParameters cost_params;
        cost_params.n_o = tracking_params.n_o;
        cost_params.units = createUnitParameters(tracking_params);

        // Get space constraints
        float min_horizontal = config_ptr->horizontal_constraint(0);
        float max_horizontal = config_ptr->horizontal_constraint(1);
        float min_vertical = config_ptr->vertical_constraint(0);
        float max_vertical = std::min(config_ptr->vertical_constraint(1), agent_ptr->max_altitude);
        Vector3r x_min = Vector3r(min_horizontal, min_horizontal, min_vertical);
        Vector3r x_max = Vector3r(max_horizontal, max_horizontal, max_vertical);

        // Create solver parameters
        PositionSolver::Parameters params = solver_params;
        params.cost_params = cost_params;
        params.x_min = x_min;
        params.x_max = x_max;

        // Initialize solver
        solver->init(solver_mode, params);

        return solver;
    }

    std::vector<CostFunctions::UnitCostParameters> AgentPositioning::createUnitParameters(const TrackingParameters& tracking_params)
    {
        std::vector<CostFunctions::UnitCostParameters> params_vector;

        // Get unit parameters each observation unit
        for (const auto& unit_params : tracking_params.observation_units_params)
        {
            CostFunctions::UnitCostParameters unit_cost_params;

            // Set unit parameters
            unit_cost_params.params = unit_params;

            // Cost function weights
            // Psi
            unit_cost_params.tau0 = 1.0f;
            unit_cost_params.tau1 = 2.0f;
            unit_cost_params.tau2 = 10.0f;
            // Lambda
            unit_cost_params.sigma0 = 1.0f;
            unit_cost_params.sigma1 = 2.0f;
            unit_cost_params.sigma2 = 10.0f;
            // Gamma
            unit_cost_params.mu = 1.0f;
            unit_cost_params.nu = 1.0f;

            params_vector.push_back(unit_cost_params);
        }

        return params_vector;
    }

} // namespace flychams::agent