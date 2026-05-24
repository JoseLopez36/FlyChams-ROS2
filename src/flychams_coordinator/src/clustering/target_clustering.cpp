#include "flychams_coordinator/clustering/target_clustering.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void TargetClustering::onModuleInit()
{
	// Get parameters from parameter server
	// Get update rate
	update_rate_ = node_->getParameterOr<float>("clustering_rate", 3.33f);
	// Get persistence parameters
	float ini_bonding_coef = node_->getParameterOr<float>("persistence.ini_bonding_coef", 1.0f);
	float max_bonding_coef = node_->getParameterOr<float>("persistence.max_bonding_coef", 1.0f);
	float bonding_coef_time_to_max = node_->getParameterOr<float>("persistence.bonding_coef_time_to_max", 100.0f);
	float max_hysteresis_ratio = node_->getParameterOr<float>("persistence.max_hysteresis_ratio", 0.4f);
	float min_hysteresis_ratio = node_->getParameterOr<float>("persistence.min_hysteresis_ratio", 0.2f);

	// Compute command timeout
	cmd_timeout_ = (1.0f / update_rate_) * 1.25f;

	// Initialize data
	clusters_.clear();
	C_.clear();
	targets_.clear();
	T_.clear();
	assignments_prev_.resize(0);
	is_first_run_ = true;

	// Create and initialize K-Means solver
	k_means_solver_ = std::make_shared<KMeansMod>();
	KMeansMod::Parameters solver_params;
	solver_params.ini_bonding_coef = ini_bonding_coef;
	solver_params.max_bonding_coef = max_bonding_coef;
	solver_params.bonding_coef_time_to_max = bonding_coef_time_to_max;
	solver_params.max_hysteresis_ratio = max_hysteresis_ratio;
	solver_params.min_hysteresis_ratio = min_hysteresis_ratio;
	k_means_solver_->init(solver_params);

	// Set update timer
	last_update_time_ = node_->now();
	update_timer_ = node_->createTimer(update_rate_, std::bind(&TargetClustering::update, this));
}

void TargetClustering::onModuleShutdown()
{
	// Destroy K-Means solver
	k_means_solver_->destroy();
	// Destroy clusters and targets
	clusters_.clear();
	C_.clear();
	targets_.clear();
	T_.clear();
	// Destroy update timer
	update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLIC METHODS: Public methods for adding/removing clusters and targets
// ════════════════════════════════════════════════════════════════════════════

void TargetClustering::addCluster(const ID& cluster_id)
{
	// Create and add cluster
	clusters_.insert({ cluster_id, Cluster() });
	C_.insert(cluster_id); // Add cluster to ordered set

	// Create cluster assignment publisher
	clusters_[cluster_id].assignment_pub = node_->createClusterAssignmentPublisher(cluster_id);
}

void TargetClustering::removeCluster(const ID& cluster_id)
{
	// Remove cluster from map
	clusters_.erase(cluster_id);
	C_.erase(cluster_id); // Remove cluster from ordered set
}

void TargetClustering::addTarget(const ID& target_id)
{
	// Create and add target
	targets_.insert({ target_id, Target() });
	T_.insert(target_id); // Add target to ordered set

	// Add target to previous assignments
	assignments_prev_.resize(assignments_prev_.size() + 1);
	assignments_prev_.setConstant(-1);

	// Create target true position subscriber
	targets_[target_id].position_sub = node_->createTargetPositionSubscriber(target_id,
		[this, target_id](const PointStampedMsg::SharedPtr msg)
		{
			this->targetPositionCallback(target_id, msg);
		}, node_->getSubscriptionOptions());
}

void TargetClustering::removeTarget(const ID& target_id)
{
	// Remove target from map
	targets_.erase(target_id);
	T_.erase(target_id); // Remove target from ordered set
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void TargetClustering::targetPositionCallback(const ID& target_id, const PointStampedMsg::SharedPtr msg)
{
	// Update target position
	targets_[target_id].position = msg->point;
	targets_[target_id].has_position = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update clustering
// ════════════════════════════════════════════════════════════════════════════

void TargetClustering::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Target clustering: Skipping update due to invalid status");
        return;
    }

	// Compute time step
	auto current_time = node_->now();
	float dt = (current_time - last_update_time_).seconds();
	last_update_time_ = current_time;

	// Limit dt to prevent extreme values after pauses
	dt = std::min(dt, cmd_timeout_);

	// Get vectors of target data with ordered data
	// It is important that the data is ordered according to the ordered set T_,
	// since the K-Means solver assumes that the data follows the same order always.
	// Targets
	int n = static_cast<int>(T_.size());
	Matrix3Xr tab_P(3, n);
	int i = 0;
	for (const auto& target_id : T_)
	{
		const auto& target = targets_[target_id];
		tab_P.col(i) = node_->fromMsg(target.position);
		i++;
	}

	// Get number of clusters (capped to the number of targets)
	int K = std::min(static_cast<int>(C_.size()), n);

	// Get clustering mode
	KMeansMod::Mode mode = KMeansMod::Mode::CONSISTENT_AND_PERSISTENT;
	if (is_first_run_)
	{
		mode = KMeansMod::Mode::INITIAL;
		is_first_run_ = false;
	}

	// Perform clustering with available points
	RCLCPP_DEBUG(node_->get_logger(), "Target clustering: Performing clustering...");
	const auto& assignments = k_means_solver_->run(K, tab_P, assignments_prev_, mode, dt);
	RCLCPP_DEBUG(node_->get_logger(), "Target clustering: Clustering completed: ");
	for (int i = 0; i < n; i++)
	{
		RCLCPP_DEBUG(node_->get_logger(), "Target clustering:     %d", assignments(i));
	}

	// Update previous assignments
	assignments_prev_ = assignments;

	// Create and publish an assignment message for each active cluster (only indices 0..K-1)
	int k = 0;
	for (const auto& cluster_id : C_)
	{
		// Stop once we have covered all clusters used by K-Means
		if (k >= K)
		{
			break;
		}

		// Create message
		ClusterAssignmentMsg msg;
		msg.header.stamp = node_->get_clock()->now();

		// Get assignment
		for (int i = 0; i < n; i++)
		{
			const int& c = assignments(i);
			if (c == k)
			{
				const std::string target_id = *std::next(T_.begin(), i);
				msg.target_ids.push_back(target_id);
			}
		}

		// Publish
		clusters_[cluster_id].assignment_pub->publish(msg);

		k++;
	}
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool TargetClustering::checkStatus()
{
    // Check 1: Mission must be active
    if (!node_->isMissionActive())
    {
        RCLCPP_INFO(node_->get_logger(), "Target clustering: Mission is not active");
        return false;
    }

    // Check 2: Fleet must be active
    if (!node_->isFleetActive())
    {
        RCLCPP_INFO(node_->get_logger(), "Target clustering: Fleet is not active");
        return false;
    }

	// Check 3: There must be at least one cluster and one target
	if (C_.empty() || T_.empty())
	{
		RCLCPP_INFO(node_->get_logger(), "Target clustering: No clusters or targets available");
		return false;
	}

	// Check 4: All targets must have a defined position
	for (const auto& [target_id, target] : targets_)
	{
		if (!target.has_position)
		{
			RCLCPP_INFO(node_->get_logger(), "Target clustering: Target %s has no position", target_id.c_str());
			return false;
		}
	}
    
    // All checks passed
    return true;
}