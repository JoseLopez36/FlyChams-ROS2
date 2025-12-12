#pragma once

// Base module include
#include "flychams_core/base/base_module.hpp"

namespace flychams::simulation
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Factory for creating and updating markers
     *
     * @details
     * This class is responsible for creating and updating various
     * markers.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-02-27
     * ════════════════════════════════════════════════════════════════
     */
    class MarkerFactory : public core::BaseModule
    {
    public: // Constructor/Destructor
        MarkerFactory(core::NodePtr node, core::SettingsTools::SharedPtr config_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
            : BaseModule(node, config_tools, topic_tools, transform_tools, module_cb_group)
        {
            init();
        }

    protected: // Overrides
        void onInit() override;
        void onShutdown() override;

    public: // Types
        using SharedPtr = std::shared_ptr<MarkerFactory>;
        struct Agent
        {
            // Metrics message
            core::MarkerArrayMsg markers;
            // Subscribers
            core::SubscriberPtr<core::PointStampedMsg> position_sub;
            core::SubscriberPtr<core::PointStampedMsg> position_setpoint_sub;
            // Publisher
            core::PublisherPtr<core::MarkerArrayMsg> marker_pub;
            // Constructor
            Agent()
                : markers(), position_sub(), position_setpoint_sub(), marker_pub()
            {
            }
        };
        struct Target
        {
            // Metrics message
            core::MarkerArrayMsg markers;
            // Subscribers
            core::SubscriberPtr<core::PointStampedMsg> position_sub;
            // Publisher
            core::PublisherPtr<core::MarkerArrayMsg> marker_pub;
            // Constructor
            Target()
                : markers(), position_sub(), marker_pub()
            {
            }
        };
        struct Cluster
        {
            // Metrics message
            core::MarkerArrayMsg markers;
            // Subscriber
            core::SubscriberPtr<core::ClusterGeometryMsg> geometry_sub;
            // Publisher
            core::PublisherPtr<core::MarkerArrayMsg> marker_pub;
            // Constructor
            Cluster()
                : markers(), geometry_sub(), marker_pub()
            {
            }
        };

    private: // Parameters
        float update_rate_;
        // Marker constants
        static constexpr float BASE_MARKER_SIZE = 2.5f;
        static constexpr float BASE_LINE_WIDTH = 0.6f;
        static constexpr float BASE_MARKER_ALPHA = 0.8f;

    private: // Data
        // Agents
        std::unordered_map<core::ID, Agent> agents_;
        // Targets
        std::unordered_map<core::ID, Target> targets_;
        // Clusters
        std::unordered_map<core::ID, Cluster> clusters_;

    public: // Public methods
        void addAgent(const core::ID& agent_id);
        void addTarget(const core::ID& target_id);
        void addCluster(const core::ID& cluster_id);
        void removeAgent(const core::ID& agent_id);
        void removeTarget(const core::ID& target_id);
        void removeCluster(const core::ID& cluster_id);

    private: // Callbacks
        void agentPositionCallback(const core::ID& agent_id, const core::PointStampedMsg::SharedPtr msg);
        void agentPositionSetpointCallback(const core::ID& agent_id, const core::PointStampedMsg::SharedPtr msg);
        void targetPositionCallback(const core::ID& target_id, const core::PointStampedMsg::SharedPtr msg);
        void clusterGeometryCallback(const core::ID& cluster_id, const core::ClusterGeometryMsg::SharedPtr msg);

    private: // Marker management
        void update();

    private: // Marker methods
        void createAgentMarkers(core::MarkerArrayMsg& markers);
        void createTargetMarkers(core::MarkerArrayMsg& markers);
        void createClusterMarkers(core::MarkerArrayMsg& markers);

    private: // ROS components
        // Timer
        core::TimerPtr update_timer_;
    };

} // namespace flychams::simulation