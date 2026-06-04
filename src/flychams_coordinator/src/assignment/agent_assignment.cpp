#include "flychams_coordinator/assignment/agent_assignment.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void AgentAssignment::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate and minimum inter-solve rate
    update_rate_ = node_->getParameterOr<float>("update_rate", 20.0f);
    min_assignment_rate_ = node_->getParameterOr<float>("min_assignment_rate", 1.0f);
    // Get assignment mode
    AssignmentSolver::SolverMode assignment_solver_mode = static_cast<AssignmentSolver::SolverMode>(node_->getParameterOr<uint8_t>("assignment.solver_mode", 0));
    // Get assignment solver parameters
    float observation_weight = node_->getParameterOr<float>("assignment.observation_weight", 1.0f);
    float distance_weight = node_->getParameterOr<float>("assignment.distance_weight", 10.0f);
    float switch_weight = node_->getParameterOr<float>("assignment.switch_weight", 5000.0f);
    // Get position solver parameters
    position_solver_mode_ = static_cast<PositionSolver::SolverMode>(node_->getParameterOr<uint8_t>("positioning.solver_mode", 0));
    // Get generic position solver parameters
    position_solver_params_.eps = node_->getParameterOr<float>("positioning.eps", 1.0e-1f);
    position_solver_params_.tol = node_->getParameterOr<float>("positioning.convergence_tolerance", 1.0e-5f);
    position_solver_params_.max_iter = node_->getParameterOr<int>("positioning.max_iterations", 100);
    // Get PSO parameters
    position_solver_params_.num_particles = node_->getParameterOr<int>("positioning.num_particles", 50);
    position_solver_params_.w_max = node_->getParameterOr<float>("positioning.w_max", 0.4f);
    position_solver_params_.w_min = node_->getParameterOr<float>("positioning.w_min", 0.1f);
    position_solver_params_.c1 = node_->getParameterOr<float>("positioning.c1", 1.0f);
    position_solver_params_.c2 = node_->getParameterOr<float>("positioning.c2", 1.0f);
    position_solver_params_.stagnation_limit = node_->getParameterOr<int>("positioning.stagnation_limit", 5);
    // Get ALC-PSO parameters
    position_solver_params_.max_lifespan = node_->getParameterOr<int>("positioning.max_lifespan", 60);
    position_solver_params_.num_challenger_tests = node_->getParameterOr<int>("positioning.num_challenger_tests", 10);
    // Get Nesterov parameters
    position_solver_params_.lipschitz_constant = node_->getParameterOr<float>("positioning.lipschitz_constant", 0.0f);

    // Initialize data
    agents_.clear();
    A_.clear();
    clusters_.clear();
    T_.clear();
    X_prev_.resize(0);

    // Get relevant transform frames
    world_frame_ = node_->getGlobalFrame();
    central_optical_frame_map_.clear();

    // Create and initialize assignment solver
    // Note: Position solvers will be created when adding agents
    solver_ = std::make_shared<AssignmentSolver>();
    AssignmentSolver::Parameters solver_params;
    solver_params.observation_weight = observation_weight;
    solver_params.distance_weight = distance_weight;
    solver_params.switch_weight = switch_weight;
    solver_->init(assignment_solver_mode, solver_params);

    // Create publishers for assignment benchmarking
    solve_duration_pub_ = node_->createAssignmentSolveDurationPublisher();
    node_count_pub_ = node_->createAssignmentNodeCountPublisher();

    // Set update timer
    last_solve_time_ = common::Time(0, 0, RCL_ROS_TIME);
    update_timer_ = node_->createTimer(update_rate_, std::bind(&AgentAssignment::update, this));
}

void AgentAssignment::onModuleShutdown()
{
    // Wait for any in-progress async solve before destroying shared resources
    if (async_future_.valid())
    {
        async_future_.wait();
    }
    async_solvers_.clear();
    // Destroy assignment solver
    solver_->destroy();
    // Destroy agents and clusters
    agents_.clear();
    A_.clear();
    clusters_.clear();
    T_.clear();
    // Destroy publishers
    solve_duration_pub_.reset();
    node_count_pub_.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLIC METHODS: Public methods for adding/removing clusters and agents
// ════════════════════════════════════════════════════════════════════════════

void AgentAssignment::addAgent(const ID& agent_id)
{
    // Create and add agent
    agents_.insert({ agent_id, Agent() });
    A_.insert(agent_id); // Add agent to ordered set

    // Define tracking units IDs
    const auto& tracking_params = node_->getSettings()->getTrackingParameters(agent_id);
    agents_[agent_id].tracking_unit_ids.resize(tracking_params.n_t);
    int t_u = 0;
    for (int i = 1; i < tracking_params.n_o; i++)
    {
        agents_[agent_id].tracking_unit_ids[t_u] = tracking_params.observation_units_params[i].id;
        t_u++;
    }

    // Get central observation unit optical frame
    central_optical_frame_map_.insert({ agent_id,
        node_->getCameraOpticalFrame(agent_id, tracking_params.observation_units_params[0].id) });

    // Create and initialize position solver
    agents_[agent_id].position_solver = createPositionSolver(agent_id, position_solver_params_, position_solver_mode_);

    // Add tracking units to previous assignments
    X_prev_.resize(X_prev_.size() + agents_[agent_id].position_solver->getUnitCount() - 1);
    X_prev_.setConstant(-1);

    // Create agent position subscriber
    agents_[agent_id].position_sub = node_->createAgentGlobalPositionSubscriber(agent_id,
        [this, agent_id](const PointStampedMsg::SharedPtr msg)
        {
            this->agentPositionCallback(agent_id, msg);
        }, node_->getSubscriptionOptions());

    // Create agent assignment publisher
    agents_[agent_id].assignment_pub = node_->createAgentAssignmentPublisher(agent_id);
}

void AgentAssignment::removeAgent(const ID& agent_id)
{
    // Remove agent from map
    agents_.erase(agent_id);
    A_.erase(agent_id); // Remove agent from ordered set
}

void AgentAssignment::addCluster(const ID& cluster_id)
{
    // Create and add cluster
    clusters_.insert({ cluster_id, Cluster() });
    T_.insert(cluster_id); // Add cluster to ordered set

    // Create cluster geometry subscriber
    clusters_[cluster_id].geometry_sub = node_->createClusterGeometrySubscriber(cluster_id,
        [this, cluster_id](const ClusterGeometryMsg::SharedPtr msg)
        {
            this->clusterGeometryCallback(cluster_id, msg);
        }, node_->getSubscriptionOptions());
}

void AgentAssignment::removeCluster(const ID& cluster_id)
{
    // Remove cluster from map
    clusters_.erase(cluster_id);
    T_.erase(cluster_id); // Remove cluster from ordered set
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void AgentAssignment::clusterGeometryCallback(const ID& cluster_id, const ClusterGeometryMsg::SharedPtr msg)
{
    // Update cluster geometry
    clusters_[cluster_id].center = msg->center;
    clusters_[cluster_id].radius = msg->radius;
    clusters_[cluster_id].has_geometry = true;
}

void AgentAssignment::agentPositionCallback(const ID& agent_id, const PointStampedMsg::SharedPtr msg)
{
    // Update agent position
    agents_[agent_id].position = msg->point;
    agents_[agent_id].has_position = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update assignment
// ════════════════════════════════════════════════════════════════════════════

void AgentAssignment::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Agent assignment: Skipping update due to invalid status");
        return;
    }

    // ── Part 1: Process a completed async solve ───────────────────────────────
    if (async_future_.valid())
    {
        if (async_future_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            // Solve is still running — keep the executor thread free
            return;
        }

        // Retrieve result from async future
        const auto& [X, time_elapsed_ms, node_count] = async_future_.get();

        // Publish result
        publishResult(X, time_elapsed_ms, node_count);

        return;
    }

    // ── Part 2: Prepare data for async solve ─────────────────────────────────────
    // Agents data
    int n_agents = static_cast<int>(A_.size());
    Matrix3Xr tab_x(3, n_agents);
    std::vector<Matrix4r> wTcentral_array(n_agents);
    async_solvers_.resize(n_agents);
    async_agent_order_.clear();
    int k = 0;
    for (const auto& agent_id : A_)
    {
        async_agent_order_.push_back(agent_id);
        const auto& agent = agents_[agent_id];
        tab_x.col(k) = node_->fromMsg(agent.position);
        const TransformMsg& wTcentral_msg = node_->getTransform(world_frame_, central_optical_frame_map_[agent_id]);
        wTcentral_array[k] = node_->fromMsg(wTcentral_msg);
        async_solvers_[k] = agent.position_solver;
        k++;
    }

    // Clusters data
    int n_clusters = static_cast<int>(T_.size());
    Matrix3Xr tab_P(3, n_clusters);
    RowVectorXr tab_r(n_clusters);
    async_cluster_order_.clear();
    int i = 0;
    for (const auto& cluster_id : T_)
    {
        async_cluster_order_.push_back(cluster_id);
        const auto& cluster = clusters_[cluster_id];
        tab_P.col(i) = node_->fromMsg(cluster.center);
        tab_r(i) = cluster.radius;
        i++;
    }

    // Auxiliary copy of X_prev_ so the lambda owns its own copy
    RowVectorXi X_prev_aux = X_prev_;

    // ── Part 3: Launch async solve ─────────────────────────────────────
    // Enforce minimum inter-solve interval
    if ((node_->now() - last_solve_time_).seconds() < 1.0 / min_assignment_rate_)
    {
        return;
    }

    RCLCPP_DEBUG(node_->get_logger(), "Agent assignment: Performing agent assignment...");
    last_solve_time_ = node_->now();
    async_future_ = std::async(std::launch::async,
        [this,
         tab_x           = std::move(tab_x),
         tab_P           = std::move(tab_P),
         tab_r           = std::move(tab_r),
         X_prev_aux      = std::move(X_prev_aux),
         wTcentral_array = std::move(wTcentral_array)]() mutable
        {
            // Solve assignment
            const auto start = std::chrono::high_resolution_clock::now();
            const auto& [X, node_count] = solver_->run(tab_x, tab_P, tab_r, X_prev_aux, wTcentral_array, async_solvers_);
            float time_elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() / 1000.0f;

            return std::make_tuple(X, time_elapsed_ms, node_count);
        });
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool AgentAssignment::checkStatus()
{
    // Check 1: Mission must be active
    if (!node_->isMissionActive())
    {
        RCLCPP_INFO(node_->get_logger(), "Agent assignment: Mission is not active");
        return false;
    }

    // Check 2: Fleet must be active
    if (!node_->isFleetActive())
    {
        RCLCPP_INFO(node_->get_logger(), "Agent assignment: Fleet is not active");
        return false;
    }

    // Check 3: All agents must have a defined position
    for (const auto& [agent_id, agent] : agents_)
    {
        if (!agent.has_position)
        {
            RCLCPP_INFO(node_->get_logger(), "Agent assignment: Agent %s has no position", agent_id.c_str());
            return false;
        }
    }

    // Check 4: All clusters must have a defined geometry
    for (const auto& [cluster_id, cluster] : clusters_)
    {
        if (!cluster.has_geometry)
        {
            RCLCPP_INFO(node_->get_logger(), "Agent assignment: Cluster %s has no geometry", cluster_id.c_str());
            return false;
        }
    }
    
    // All checks passed
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLISH RESULT
// ════════════════════════════════════════════════════════════════════════════

void AgentAssignment::publishResult(const common::RowVectorXi& X, float time_elapsed_ms, int node_count)
{
    // Update previous assignment
    X_prev_.resize(X.size());
    X_prev_ = X;

    // Create and publish an assignment message for each agent
    int k = 0;
    int t = 0;
    for (const auto& agent_id : async_agent_order_)
    {
        // Create message
        AgentAssignmentMsg msg;
        msg.header.stamp = node_->get_clock()->now();

        // Get assignment
        int n = async_solvers_[k]->getUnitCount() - 1;
        for (int i = 0; i < n; i++)
        {
            const std::string unit_id = agents_[agent_id].tracking_unit_ids[i];
            const std::string cluster_id = async_cluster_order_[X(t)];
            msg.unit_ids.push_back(unit_id);
            msg.cluster_ids.push_back(cluster_id);
            t++;
        }

        // Publish
        agents_[agent_id].assignment_pub->publish(msg);

        // Log assignment
        RCLCPP_DEBUG(node_->get_logger(), "Agent assignment: Agent %s assigned to %d clusters", agent_id.c_str(), n);
        for (int i = 0; i < n; i++)
        {
            RCLCPP_DEBUG(node_->get_logger(), "Agent assignment:     - Unit %s    - Cluster %s", msg.unit_ids[i].c_str(), msg.cluster_ids[i].c_str());
        }

        k++;
    }

    // Publish solve duration
    std_msgs::msg::Float32 duration_msg;
    duration_msg.data = time_elapsed_ms;
    solve_duration_pub_->publish(duration_msg);

    // Publish evaluated node count
    std_msgs::msg::Int32 node_count_msg;
    node_count_msg.data = node_count;
    node_count_pub_->publish(node_count_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// UTILITY METHODS
// ════════════════════════════════════════════════════════════════════════════

PositionSolver::SharedPtr AgentAssignment::createPositionSolver(const std::string& agent_id, const PositionSolver::Parameters& solver_params, const PositionSolver::SolverMode& solver_mode)
{
    // Create solver instance
    PositionSolver::SharedPtr solver = std::make_shared<PositionSolver>();

    // Get config
    const auto& config_ptr = node_->getSettings()->getConfig();
    const auto& agent_ptr = node_->getSettings()->getAgent(agent_id);
    const auto& tracking_params = node_->getSettings()->getTrackingParameters(agent_id);

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

std::vector<CostFunctions::UnitCostParameters> AgentAssignment::createUnitParameters(const TrackingParameters& tracking_params)
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