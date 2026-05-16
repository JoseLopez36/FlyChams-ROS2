#include "flychams_coordinator/state/mission_state.hpp"

using namespace flychams::common;

using namespace flychams::coordinator;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void MissionState::onModuleInit()
{
    // Get parameters from parameter server
    update_rate_ = node_->getParameterOr<float>("mission_state_rate", 1.0f);

    // Initialize data
    agents_.clear();
    fleet_status_ = FleetStatus::IDLE;
    has_fleet_status_ = false;
    mission_status_ = MissionStatus::READY;
    mission_active_ = false;
    mission_time_ = 0.0f;
    fleet_ready_ = false;

    // Create fleet status subscriber
    fleet_status_sub_ = node_->createFleetStatusSubscriber(
        std::bind(&MissionState::fleetStatusCallback, this, std::placeholders::_1), node_->getSubscriptionOptions());

    // Create mission command subscribers
    start_mission_sub_ = node_->create_subscription<BoolMsg>(
        "/flychams/coordinator/start_mission", 10,
        std::bind(&MissionState::startMissionCallback, this, std::placeholders::_1));
    pause_mission_sub_ = node_->create_subscription<BoolMsg>(
        "/flychams/coordinator/pause_mission", 10,
        std::bind(&MissionState::pauseMissionCallback, this, std::placeholders::_1));
    abort_mission_sub_ = node_->create_subscription<BoolMsg>(
        "/flychams/coordinator/abort_mission", 10,
        std::bind(&MissionState::abortMissionCallback, this, std::placeholders::_1));

    // Create mission status publisher
    mission_status_pub_ = node_->createMissionStatusPublisher();

    // Set update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&MissionState::update, this));
}

void MissionState::onModuleShutdown()
{
    // Destroy update timer
    update_timer_.reset();
    // Destroy agents
    agents_.clear();
    // Destroy publishers
    mission_status_pub_.reset();
    // Destroy subscribers
    fleet_status_sub_.reset();
    start_mission_sub_.reset();
    pause_mission_sub_.reset();
    abort_mission_sub_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// PUBLIC METHODS: Dynamic element management
// ════════════════════════════════════════════════════════════════════════════

void MissionState::addAgent(const ID& agent_id)
{
    agents_.insert(agent_id);
    RCLCPP_INFO(node_->get_logger(), "Mission manager: Agent %s added", agent_id.c_str());
}

void MissionState::removeAgent(const ID& agent_id)
{
    agents_.erase(agent_id);
    RCLCPP_INFO(node_->get_logger(), "Mission manager: Agent %s removed", agent_id.c_str());
}

// ════════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ════════════════════════════════════════════════════════════════════════════

void MissionState::fleetStatusCallback(const FleetStatusMsg::SharedPtr msg)
{
    fleet_status_ = static_cast<FleetStatus>(msg->status);
    has_fleet_status_ = true;

    // Fleet is ready when all agents are idle (ready to start mission)
    fleet_ready_ = msg->all_agents_idle && !agents_.empty();
}

void MissionState::startMissionCallback(const BoolMsg::SharedPtr msg)
{
    if (mission_status_ == MissionStatus::READY && fleet_ready_)
    {
        mission_status_ = MissionStatus::ACTIVE;
        mission_active_ = true;
        mission_start_time_ = std::chrono::steady_clock::now();
        mission_time_ = 0.0f;
        RCLCPP_INFO(node_->get_logger(), "Mission started");
    }
    else if (mission_status_ == MissionStatus::PAUSED)
    {
        // Resume from pause
        mission_status_ = MissionStatus::ACTIVE;
        mission_active_ = true;
        RCLCPP_INFO(node_->get_logger(), "Mission resumed");
    }
    else
    {
        RCLCPP_WARN(node_->get_logger(), "Cannot start mission: status=%d, fleet_ready=%d",
                    static_cast<int>(mission_status_), fleet_ready_);
    }
}

void MissionState::pauseMissionCallback(const BoolMsg::SharedPtr msg)
{
    if (mission_status_ == MissionStatus::ACTIVE)
    {
        mission_status_ = MissionStatus::PAUSED;
        mission_active_ = false;
        RCLCPP_INFO(node_->get_logger(), "Mission paused");
    }
    else
    {
        RCLCPP_WARN(node_->get_logger(), "Cannot pause mission: not active (status=%d)",
                    static_cast<int>(mission_status_));
    }
}

void MissionState::abortMissionCallback(const BoolMsg::SharedPtr msg)
{
    if (mission_status_ == MissionStatus::ACTIVE || mission_status_ == MissionStatus::PAUSED)
    {
        mission_status_ = MissionStatus::ABORTED;
        mission_active_ = false;
        RCLCPP_INFO(node_->get_logger(), "Mission aborted");
    }
    else
    {
        RCLCPP_WARN(node_->get_logger(), "Cannot abort mission: not active or paused (status=%d)",
                    static_cast<int>(mission_status_));
    }
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Publish mission state
// ════════════════════════════════════════════════════════════════════════════

void MissionState::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_WARN(node_->get_logger(), "Mission state: Skipping update due to invalid status");
        return;
    }

    // Update mission time if active
    if (mission_active_)
    {
        auto now = std::chrono::steady_clock::now();
        mission_time_ = std::chrono::duration<float>(now - mission_start_time_).count();
    }

    // Build active agents list
    std::vector<std::string> agent_list;
    if (mission_active_ || mission_status_ == MissionStatus::PAUSED)
    {
        for (const auto& agent_id : agents_)
        {
            agent_list.push_back(agent_id);
        }
    }

    // Publish MissionStatus
    MissionStatusMsg mission_msg;
    mission_msg.header.stamp = node_->now();
    mission_msg.status = static_cast<uint8_t>(mission_status_);
    mission_msg.fleet_ready = fleet_ready_;
    mission_msg.mission_time = mission_time_;
    mission_msg.agents = agent_list;
    mission_status_pub_->publish(mission_msg);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool MissionState::checkStatus()
{
    // Check 1: Fleet status is valid
    if (!has_fleet_status_)
    {
        RCLCPP_WARN(node_->get_logger(), "Mission state: Fleet status not received yet");
        return false;
    }
    
    return true;
}