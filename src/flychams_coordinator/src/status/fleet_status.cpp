#include "flychams_coordinator/fleet/fleet_status.hpp"

using namespace flychams::core;

namespace flychams::coordinator
{
    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR: Constructor and destructor
    // ════════════════════════════════════════════════════════════════════════════

    void FleetStatus::onInit()
    {
        // Get parameters from parameter server
        update_rate_ = RosUtils::getParameterOr<float>(node_, "fleet_manager_rate", 1.0f);

        // Initialize data
        agents_.clear();
        mission_state_ = MissionState::READY;
        mission_time_ = 0.0f;
        mission_start_time_ = RosUtils::now(node_);
        active_agents_.clear();

        // Create fleet/mission publishers
        fleet_status_pub_ = topic_tools_->createFleetStatusPublisher();
        mission_status_pub_ = topic_tools_->createMissionStatusPublisher();

        // Create mission command subscriber
        mission_cmd_sub_ = node_->create_subscription<StringMsg>(
            "/flychams/coordinator/mission_cmd", 10,
            [this](const StringMsg::SharedPtr msg)
            {
                this->missionCmdCallback(msg);
            }, sub_options_with_module_cb_group_);

        // Set update timer
        update_timer_ = rclcpp::create_timer(node_,
            node_->get_clock(),
            std::chrono::duration<float>(1.0f / update_rate_),
            std::bind(&FleetStatus::update, this),
            module_cb_group_);
    }

    void FleetStatus::onShutdown()
    {
        agents_.clear();
        update_timer_.reset();
        fleet_status_pub_.reset();
        mission_status_pub_.reset();
        mission_cmd_sub_.reset();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // PUBLIC METHODS: Dynamic element management
    // ════════════════════════════════════════════════════════════════════════════

    void FleetStatus::addAgent(const ID& agent_id)
    {
        agents_.insert({ agent_id, Agent() });

        agents_[agent_id].status_sub = topic_tools_->createAgentStatusSubscriber(agent_id,
            [this, agent_id](const AgentStatusMsg::SharedPtr msg)
            {
                this->agentStatusCallback(agent_id, msg);
            }, sub_options_with_module_cb_group_);

        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Agent %s added", agent_id.c_str());
    }

    void FleetStatus::removeAgent(const ID& agent_id)
    {
        agents_.erase(agent_id);
        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Agent %s removed", agent_id.c_str());
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CALLBACKS
    // ════════════════════════════════════════════════════════════════════════════

    void FleetStatus::agentStatusCallback(const ID& agent_id, const AgentStatusMsg::SharedPtr msg)
    {
        agents_[agent_id].status = static_cast<AgentStatus>(msg->status);
        agents_[agent_id].has_status = true;
    }

    void FleetStatus::missionCmdCallback(const StringMsg::SharedPtr msg)
    {
        const std::string& cmd = msg->data;
        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Mission command received: %s", cmd.c_str());

        if (cmd == "start")
        {
            if (mission_state_ == MissionState::READY)
            {
                bool fleet_ready = (computeFleetState() == FleetState::ACTIVE);
                if (fleet_ready)
                    transitionMission(MissionState::ACTIVE);
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
            if (mission_state_ == MissionState::ACTIVE)
                transitionMission(MissionState::PAUSED);
            else
                RCLCPP_WARN(node_->get_logger(), "Fleet manager: Cannot pause - mission not ACTIVE");
        }
        else if (cmd == "resume")
        {
            if (mission_state_ == MissionState::PAUSED)
                transitionMission(MissionState::ACTIVE);
            else
                RCLCPP_WARN(node_->get_logger(), "Fleet manager: Cannot resume - mission not PAUSED");
        }
        else if (cmd == "abort")
        {
            transitionMission(MissionState::ABORTED);
        }
        else if (cmd == "reset")
        {
            if (mission_state_ == MissionState::ABORTED)
                transitionMission(MissionState::READY);
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

    void FleetStatus::update()
    {
        auto now = RosUtils::now(node_);

        // Compute fleet state
        FleetState fleet_state = computeFleetState();

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
        if (any_error && mission_state_ == MissionState::ACTIVE)
        {
            RCLCPP_ERROR(node_->get_logger(), "Fleet manager: Agent error detected during ACTIVE mission - aborting");
            transitionMission(MissionState::ABORTED);
        }

        // Update mission time
        if (mission_state_ == MissionState::ACTIVE)
            mission_time_ = static_cast<float>((now - mission_start_time_).seconds());

        // Publish FleetStatus
        FleetStatusMsg fleet_msg;
        fleet_msg.header.stamp = now;
        fleet_msg.all_agents_idle = all_idle;
        fleet_msg.all_agents_active = all_active;
        fleet_msg.any_agent_error = any_error;
        fleet_msg.fleet_state = static_cast<uint8_t>(fleet_state);
        fleet_msg.agent_ids = agent_ids;
        fleet_msg.agent_statuses = agent_statuses;
        fleet_status_pub_->publish(fleet_msg);

        // Publish MissionStatus
        MissionStatusMsg mission_msg;
        mission_msg.header.stamp = now;
        mission_msg.status = static_cast<uint8_t>(mission_state_);
        mission_msg.fleet_ready = (fleet_state == FleetState::ACTIVE);
        mission_msg.mission_time = mission_time_;
        mission_msg.active_agents = active_agents_;
        mission_status_pub_->publish(mission_msg);
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATE MACHINE HELPERS
    // ════════════════════════════════════════════════════════════════════════════

    void FleetStatus::transitionMission(MissionState new_state)
    {
        RCLCPP_INFO(node_->get_logger(), "Fleet manager: Mission state transition: %d -> %d",
            static_cast<int>(mission_state_), static_cast<int>(new_state));

        mission_state_ = new_state;

        if (new_state == MissionState::ACTIVE)
        {
            mission_start_time_ = RosUtils::now(node_);
            // Build active agents list (all agents currently ACTIVE)
            active_agents_.clear();
            for (const auto& [agent_id, agent] : agents_)
            {
                if (agent.has_status && agent.status == AgentStatus::ACTIVE)
                    active_agents_.push_back(agent_id);
            }
        }
        else if (new_state == MissionState::READY)
        {
            mission_time_ = 0.0f;
            active_agents_.clear();
        }
    }

    FleetState FleetStatus::computeFleetState() const
    {
        if (agents_.empty())
            return FleetState::IDLE;

        bool all_idle = true;
        bool all_active = true;
        bool any_error = false;

        for (const auto& [agent_id, agent] : agents_)
        {
            if (!agent.has_status)
                return FleetState::MIXED;

            if (agent.status != AgentStatus::IDLE) all_idle = false;
            if (agent.status != AgentStatus::ACTIVE) all_active = false;
            if (agent.status == AgentStatus::ERROR) any_error = true;
        }

        if (any_error) return FleetState::ERROR;
        if (all_active) return FleetState::ACTIVE;
        if (all_idle) return FleetState::IDLE;
        return FleetState::MIXED;
    }

} // namespace flychams::coordinator
