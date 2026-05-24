#include "flychams_coordinator/analysis/cluster_analysis.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void ClusterAnalysis::onModuleInit()
{
	// Get parameters from parameter server
	// Get update rate
	update_rate_ = node_->getParameterOr<float>("analysis_rate", 20.0f);
	// Get circle parameters
	min_circle_radius_ = node_->getParameterOr<float>("enclosing_circle.min_circle_radius", 0.10f);
	margin_circle_radius_ = node_->getParameterOr<float>("enclosing_circle.margin_circle_radius", 0.05f);

	// Initialize data
	clusters_.clear();
	targets_.clear();

	// Set update timer
	update_timer_ = node_->createTimer(update_rate_, std::bind(&ClusterAnalysis::update, this));
}

void ClusterAnalysis::onModuleShutdown()
{
	// Destroy clusters and targets
	clusters_.clear();
	targets_.clear();
	// Destroy update timer
	update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLIC METHODS: Public methods for adding/removing clusters and targets
// ════════════════════════════════════════════════════════════════════════════

void ClusterAnalysis::addCluster(const ID& cluster_id)
{
	// Create and add cluster
	clusters_.insert({ cluster_id, Cluster() });

	// Create cluster assignment subscriber
	clusters_[cluster_id].assignment_sub = node_->createClusterAssignmentSubscriber(cluster_id,
		[this, cluster_id](const ClusterAssignmentMsg::SharedPtr msg)
		{
			this->clusterAssignmentCallback(cluster_id, msg);
		}, node_->getSubscriptionOptions());

	// Create cluster geometry publisher
	clusters_[cluster_id].geometry_pub = node_->createClusterGeometryPublisher(cluster_id);
}

void ClusterAnalysis::removeCluster(const ID& cluster_id)
{
	// Remove cluster from map
	clusters_.erase(cluster_id);
}

void ClusterAnalysis::addTarget(const ID& target_id)
{
	// Create and add target
	targets_.insert({ target_id, Target() });

	// Create target true position subscriber
	targets_[target_id].position_sub = node_->createTargetPositionSubscriber(target_id,
		[this, target_id](const PointStampedMsg::SharedPtr msg)
		{
			this->targetPositionCallback(target_id, msg);
		}, node_->getSubscriptionOptions());
}

void ClusterAnalysis::removeTarget(const ID& target_id)
{
	// Remove target from map
	targets_.erase(target_id);
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS: Callback functions
// ════════════════════════════════════════════════════════════════════════════

void ClusterAnalysis::clusterAssignmentCallback(const ID& cluster_id, const ClusterAssignmentMsg::SharedPtr msg)
{
	// Update cluster assignment
	clusters_[cluster_id].assignment = msg->target_ids;
	clusters_[cluster_id].has_assignment = true;
}

void ClusterAnalysis::targetPositionCallback(const ID& target_id, const PointStampedMsg::SharedPtr msg)
{
	// Update target position
	targets_[target_id].position = msg->point;
	targets_[target_id].has_position = true;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update analysis
// ════════════════════════════════════════════════════════════════════════════

void ClusterAnalysis::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Cluster analysis: Skipping update due to invalid status");
        return;
    }

	// Compute cluster geometry and publish
	for (auto& [cluster_id, cluster] : clusters_)
	{
		// Skip clusters with no assigned targets
		if (cluster.assignment.empty())
		{
			continue;
		}

		// Iterate over the assignment and get the points
		int n = static_cast<int>(cluster.assignment.size());
		Matrix3Xr tab_P(3, n);
		for (int i = 0; i < n; i++)
		{
			const auto& target = targets_[cluster.assignment[i]];
			tab_P.col(i) = node_->fromMsg(target.position);
		}

		// Calculate enclosing circle (minimum enclosing circle with enforced limits)
		const auto& [center, radius] = calculateEnclosingCircle(tab_P, min_circle_radius_, margin_circle_radius_);

		// Create geometry message with calculated center and radius
		ClusterGeometryMsg msg;
		msg.header = node_->createHeader(node_->getGlobalFrame());
		msg.center.x = center.x();
		msg.center.y = center.y();
		msg.center.z = 0.0f;
		msg.radius = radius;

		// Publish cluster geometry
		cluster.geometry_pub->publish(msg);
	}
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool ClusterAnalysis::checkStatus()
{
	// Check 1: Mission must be active
	if (!node_->isMissionActive())
	{
		RCLCPP_INFO(node_->get_logger(), "Cluster analysis: Mission is not active");
		return false;
	}

	// Check 2: Fleet must be active
	if (!node_->isFleetActive())
	{
		RCLCPP_INFO(node_->get_logger(), "Cluster analysis: Fleet is not active");
		return false;
	}

	// Check 3: There must be at least one cluster and one target
	if (clusters_.empty() || targets_.empty())
	{
		RCLCPP_INFO(node_->get_logger(), "Cluster analysis: No clusters or targets available");
		return false;
	}

	// Check 4: All clusters must have a valid assignment
	for (const auto& [cluster_id, cluster] : clusters_)
	{
		if (!cluster.has_assignment)
		{
			RCLCPP_INFO(node_->get_logger(), "Cluster analysis: Cluster %s has no assignment", cluster_id.c_str());
			return false;
		}
	}

	// Check 5: All targets must have a defined position
	for (const auto& [target_id, target] : targets_)
	{
		if (!target.has_position)
		{
			RCLCPP_INFO(node_->get_logger(), "Cluster analysis: Target %s has no position", target_id.c_str());
			return false;
		}
	}

	// All checks passed
	return true;
}

// ════════════════════════════════════════════════════════════════════════════
// ANALYSIS: Analysis methods
// ════════════════════════════════════════════════════════════════════════════

std::pair<Vector2r, float> ClusterAnalysis::calculateEnclosingCircle(const Matrix3Xr& tab_P, const float& min_radius, const float& margin_radius)
{
	// Get number of points
	int n = tab_P.cols();

	// Handle edge cases
	if (n == 0)
		return { {0.0f, 0.0f}, min_radius + margin_radius };
	if (n == 1)
		return { {tab_P(0, 0), tab_P(1, 0)}, min_radius + margin_radius };

	// Minimal enclosing circle (Welzl's algorithm)
	std::vector<WelzlsCircle::Point2D> points_welzl;
	points_welzl.reserve(n);
	for (int i = 0; i < n; i++)
	{
		points_welzl.push_back({ tab_P(0, i), tab_P(1, i) });
	}
	WelzlsCircle::Circle mec = WelzlsCircle::welzl(points_welzl);

	// Set the enclosing radius, ensuring it's at least the minimum radius
	mec.R = std::max(mec.R, min_radius) + margin_radius;

	return { {mec.C.x, mec.C.y}, mec.R };
}