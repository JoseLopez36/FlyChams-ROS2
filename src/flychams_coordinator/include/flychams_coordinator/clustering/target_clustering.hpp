#pragma once

// Utils include
#include "flychams_common/clustering/k_means_mod.hpp"

// Base module include
#include "flychams_common/base/base_status_discoverer_module.hpp"

// Base node include
#include "flychams_common/base/base_status_discoverer_node.hpp"

namespace flychams::coordinator
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Clustering of targets
	 *
	 * @details
	 * This class is responsible for clustering targets using K-Means
	 * modified algorithm. It also performs cluster analysis to determine
	 * the minimal enclosing circle and other characteristics.
	 *
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-02-26
	 * ════════════════════════════════════════════════════════════════
	 */
	class TargetClustering : public common::BaseStatusDiscovererModule
	{
	public: // Constructor/Destructor
		TargetClustering(common::BaseStatusDiscovererNode::SharedPtr node)
			: BaseStatusDiscovererModule(node)
		{
			init();
		}

	protected: // Overrides
		void onModuleInit() override;
		void onModuleShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<TargetClustering>;
		struct Cluster
		{
			// Publisher
			common::PublisherPtr<common::ClusterAssignmentMsg> assignment_pub;
			// Constructor
			Cluster()
				: assignment_pub()
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
		// Command timeout
		float cmd_timeout_;

	private: // Data
		// Clusters
		std::unordered_map<common::ID, Cluster> clusters_;
		std::set<common::ID> C_;
		// Targets
		std::unordered_map<common::ID, Target> targets_;
		std::set<common::ID> T_;
		// K-Means clustering data
		common::RowVectorXi assignments_prev_;
		bool is_first_run_;
		// K-Means clustering solver
		common::KMeansMod::SharedPtr k_means_solver_;
		// Time step
		common::Time last_update_time_;

	public: // Public methods
		void addCluster(const common::ID& cluster_id);
		void addTarget(const common::ID& target_id);
		void removeCluster(const common::ID& cluster_id);
		void removeTarget(const common::ID& target_id);

	private: // Callbacks
		void targetPositionCallback(const common::ID& target_id, const common::PointStampedMsg::SharedPtr msg);

	private: // Clustering management
		void update();

	private: // ROS components
		// Timer
		common::TimerPtr update_timer_;
	};

} // namespace flychams::coordinator