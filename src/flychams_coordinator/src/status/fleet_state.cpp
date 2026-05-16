#include "flychams_coordinator/status/fleet_state.hpp"

using namespace flychams::core;

namespace flychams::coordinator
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void FleetState::onModuleInit()
    {
        // Get parameters from parameter server
        update_rate_ = node_->getParameterOr<float>("fleet_manager_rate", 1.0f);

        // Initialize data
        agents_.clear();
        mission_status_ = MissionStatus::READY;
        mission_time_ = 0.0f;
        mission_start_time_ = node_->now();
        active_agents_.clear();

        // Create mission command subscriber
        mission_cmd_sub_ = node_->create_subscription<StringMsg>(
            "/flychams/coordinator/mission_cmd", 10,
            [this](const StringMsg::SharedPtr msg)
            {
                this->missionCmdCallback(msg);
            }, node_->getSubscriptionOptions());

        // Create fleet/mission publishers
        fleet_status_pub_ = node_->createFleetStatusPublisher();
        mission_status_pub_ = node_->createMissionStatusPublisher();

        // Set update timer
        update_timer_ = node_->createTimer(update_rate_, std::bind(&FleetState::update, this));
    }

    void FleetState::onModuleShutdown()
    {
        // Destroy update timer
        update_timer_.reset();
        // Destroy agents
        agents_.clear();
        // Destroy publishers
        fleet_status_pub_.reset();
        mission_status_pub_.reset();
        // Destroy subscribers
        mission_cmd_sub_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // PUBLIC METHODS: Dynamic element management
    // ════════════════════════════════════════════════════════════════════════════

    void FleetState::addAgent(const ID& agent_id)
    {
        agents_.insert({ agent_id, Agent() });

        agents_[agent_id].status_sub = node_->createAgentStatusSubscriber(agent_id,
            [this, agent_id](const AgentStatusMsg::SharedPtr msg)
            {
                this->agentStatusCallback(agent_id, msg);
            }, node_->getSubscriptionOptions());

        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Agent %s added", agent_id.c_str());
    }

    void FleetState::removeAgent(const ID& agent_id)
    {
        agents_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Agent %s removed", agent_id.c_str());
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS
    // ════════════════════════════════════════════════════════════════════════════

    void FleetState::agentStatusCallback(const ID& agent_id, const AgentStatusMsg::SharedPtr msg)
    {
        agents_[agent_id].status = static_cast<AgentStatus>(msg->status);
        agents_[agent_id].has_status = true;
    }

    void FleetState::missionCmdCallback(const StringMsg::SharedPtr msg)
    {
        const std::string& cmd = msg->data;
        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Mission command received: %s", cmd.c_str());

        if (cmd == "start")
        {
            if (mission_status_ == MissionStatus::READY)
            {
                bool fleet_ready = (computeFleetStatus() == FleetStatus::ACTIVE);
                if (fleet_ready)
                    transitionMission(MissionStatus::ACTIVE);
                else
                    RCLCPP_WARN(node_->get_logger(), "Fleet manager: Cannot start mission - fleet not ready (not all agents ACTIVE)");
            }
            else
            {
                RCLCPP_WARN(node_->get_logger(), "Fleet manager: Cannot start mission - not in READY state");
            }
        }
        else if (cmd == "pause")
        {
            if (mission_status_ == MissionStatus::ACTIVE)
                transitionMission(MissionStatus::PAUSED);
            else
                RCLCPP_WARN(node_->get_logger(), "Fleet manager: Cannot pause - mission not ACTIVE");
        }
        else if (cmd == "resume")
        {
            if (mission_status_ == MissionStatus::PAUSED)
                transitionMission(MissionStatus::ACTIVE);
            else
                RCLCPP_WARN(node_->get_logger(), "Fleet manager: Cannot resume - mission not PAUSED");
        }
        else if (cmd == "abort")
        {
            transitionMission(MissionStatus::ABORTED);
        }
        else if (cmd == "reset")
        {
            if (mission_status_ == MissionStatus::ABORTED)
                transitionMission(MissionStatus::READY);
            else
                RCLCPP_WARN(node_->get_logger(), "Fleet manager: Cannot reset - mission not ABORTED");
        }
        else
        {
            RCLCPP_WARN(node_->get_logger(), "Fleet manager: Unknown mission command: %s", cmd.c_str());
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // UPDATE: Publish fleet and mission status
    // ════════════════════════════════════════════════════════════════════════════

    void FleetState::update()
    {
        auto now = node_->now();

        // Compute fleet state
        FleetStatus fleet_status = computeFleetStatus();

        // Build agent id and status arrays
        std::vector<std::string> agent_ids;
        std::vector<uint8_t> agent_statuses;
        bool all_idle = true;
        bool all_active = true;
        bool any_error = false;

        for (const auto& [agent_id, agent] : agents_)
        {
            agent_ids.push_back(agent_id);
            agent_statuses.push_back(static_cast<uint8_t>(agent.status));

            if (agent.status != AgentStatus::IDLE) all_idle = false;
            if (agent.status != AgentStatus::ACTIVE) all_active = false;
            if (agent.status == AgentStatus::ERROR) any_error = true;
        }

        // If any agent errors out while mission is ACTIVE, transition to ABORTED
        if (any_error && mission_status_ == MissionStatus::ACTIVE)
        {
            RCLCPP_ERROR(node_->get_logger(), "Fleet manager: Agent error detected during ACTIVE mission - aborting");
            transitionMission(MissionStatus::ABORTED);
        }

        // Update mission time
        if (mission_status_ == MissionStatus::ACTIVE)
            mission_time_ = static_cast<float>((now - mission_start_time_).seconds());

        // Publish FleetStatus
        FleetStatusMsg fleet_msg;
        fleet_msg.header.stamp = now;
        fleet_msg.status = static_cast<uint8_t>(fleet_status);
        fleet_msg.all_agents_idle = all_idle;
        fleet_msg.all_agents_active = all_active;
        fleet_msg.any_agent_error = any_error;
        fleet_msg.agent_ids = agent_ids;
        fleet_msg.agent_statuses = agent_statuses;
        fleet_status_pub_->publish(fleet_msg);

        // Publish MissionStatus
        MissionStatusMsg mission_msg;
        mission_msg.header.stamp = now;
        mission_msg.status = static_cast<uint8_t>(mission_status_);
        mission_msg.fleet_ready = (fleet_status == FleetStatus::ACTIVE);
        mission_msg.mission_time = mission_time_;
        mission_msg.active_agents = active_agents_;
        mission_status_pub_->publish(mission_msg);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATE MACHINE HELPERS
    // ════════════════════════════════════════════════════════════════════════════

    void FleetState::transitionMission(MissionStatus new_state)
    {
        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Mission state transition: %d -> %d",
            static_cast<int>(mission_status_), static_cast<int>(new_state));

        mission_status_ = new_state;

        if (new_state == MissionStatus::ACTIVE)
        {
            mission_start_time_ = node_->now();
            // Build active agents list (all agents currently ACTIVE)
            active_agents_.clear();
            for (const auto& [agent_id, agent] : agents_)
            {
                if (agent.has_status && agent.status == AgentStatus::ACTIVE)
                    active_agents_.push_back(agent_id);
            }
        }
        else if (new_state == MissionStatus::READY)
        {
            mission_time_ = 0.0f;
            active_agents_.clear();
        }
    }

    FleetStatus FleetState::computeFleetStatus() const
    {
        if (agents_.empty())
            return FleetStatus::IDLE;

        bool all_idle = true;
        bool all_active = true;
        bool any_error = false;

        for (const auto& [agent_id, agent] : agents_)
        {
            if (!agent.has_status)
                return FleetStatus::MIXED;

            if (agent.status != AgentStatus::IDLE) all_idle = false;
            if (agent.status != AgentStatus::ACTIVE) all_active = false;
            if (agent.status == AgentStatus::ERROR) any_error = true;
        }

        if (any_error) return FleetStatus::ERROR;
        if (all_active) return FleetStatus::ACTIVE;
        if (all_idle) return FleetStatus::IDLE;
        return FleetStatus::MIXED;
    }

} // namespace flychams::coordinator
