#pragma once

// Utils include
#include "flychams_common/clustering/welzls_circle.hpp"

// Base module include
#include "flychams_common/base/base_status_discoverer_module.hpp"

// Base node include
#include "flychams_common/base/base_status_discoverer_node.hpp"

namespace flychams::coordinator
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Cluster analysis implementation
	 *
	 * @details
	 * This class implements a cluster analysis.
	 * It provides methods for calculating cluster data (such as
	 * centroid or enclosing circle).
	 *
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-02-26
	 * ════════════════════════════════════════════════════════════════
	 */
	class ClusterAnalysis : public common::BaseStatusDiscovererModule
	{
	public: // Constructor/Destructor
		ClusterAnalysis(common::BaseStatusDiscovererNode::SharedPtr node)
			: BaseStatusDiscovererModule(node)
		{
			init();
		}

	protected: // Overrides
		void onModuleInit() override;
		void onModuleShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<ClusterAnalysis>;
		struct Cluster
		{
			// Assignment data
			std::vector<common::ID> assignment;
			bool has_assignment;
			// Subscriber
			common::SubscriberPtr<common::ClusterAssignmentMsg> assignment_sub;
			// Publisher
			common::PublisherPtr<common::ClusterGeometryMsg> geometry_pub;
			// Constructor
			Cluster()
				: assignment(), has_assignment(false), assignment_sub(), geometry_pub()
			{
			}
		};
		struct Target
		{
			// Position data
			common::PointMsg position;
			bool has_position;
			// Subscriber
			common::SubscriberPtr<common::PointStampedMsg> position_sub;
			// Constructor
			Target()
				: position(), has_position(false), position_sub()
			{
			}
		};

	private: // Parameters
		float update_rate_;
		// Enclosing circle parameters
		float min_circle_radius_;
		float margin_circle_radius_;

	private: // Data
		// Clusters
		std::unordered_map<common::ID, Cluster> clusters_;
		// Targets
		std::unordered_map<common::ID, Target> targets_;

	public: // Public methods
		void addCluster(const common::ID& cluster_id);
		void addTarget(const common::ID& target_id);
		void removeCluster(const common::ID& cluster_id);
		void removeTarget(const common::ID& target_id);

	private: // Callbacks
		void clusterAssignmentCallback(const common::ID& cluster_id, const common::ClusterAssignmentMsg::SharedPtr msg);
		void targetPositionCallback(const common::ID& target_id, const common::PointStampedMsg::SharedPtr msg);

	private: // Analysis management
		void update();
		bool checkStatus();

	private: // Analysis methods
		std::pair<common::Vector2r, float> calculateEnclosingCircle(const common::Matrix3Xr& tab_P, const float& min_radius, const float& margin_radius);

	private: // ROS components
		// Timer
		common::TimerPtr update_timer_;
	};

} // namespace flychams::coordinator