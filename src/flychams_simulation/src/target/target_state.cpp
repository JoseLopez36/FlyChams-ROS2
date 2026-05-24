#include "flychams_simulation/target/target_state.hpp"

using namespace flychams::common;

using namespace flychams::simulation;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void TargetState::onModuleInit()
{
    // Get parameters from parameter server
    // Get update rate
    update_rate_ = node_->getParameterOr<float>("update_rate", 20.0f);

    // Compute command timeout
    cmd_timeout_ = (1.0f / update_rate_) * 1.25f;

    // Initialize data
    trajectory_ = Trajectory();
    target_ = Target();

    // Parse trajectory
    const auto& root = node_->getSettings()->getSystem().trajectory_root;
    const auto& folder = node_->getSettings()->getTarget(target_id_)->trajectory_folder;
    const auto& index = node_->getSettings()->getTarget(target_id_)->target_index;
    const auto& path = root + "/" + folder + "/" + "trajectory_" + std::to_string(index) + ".csv";
    RCLCPP_INFO(node_->get_logger(), "Target state: Parsing trajectory for target %s with path %s", target_id_.c_str(), path.c_str());
    trajectory_.points = TrajectoryParser::parse(path);
    trajectory_.current_idx = 0;
    trajectory_.num_points = static_cast<int>(trajectory_.points.size());
    trajectory_.reverse = false;

    // Check if trajectory is empty
    if (trajectory_.num_points == 0)
    {
        RCLCPP_ERROR(node_->get_logger(), "Target state: No trajectory data available for target %s",
            target_id_.c_str());
        rclcpp::shutdown();
        return;
    }

    // Initialize target position
    target_.position.header = node_->createHeader(node_->getGlobalFrame());
    target_.position.point.x = trajectory_.points[0].x;
    target_.position.point.y = trajectory_.points[0].y;
    target_.position.point.z = trajectory_.points[0].z;

    // Initialize target position publisher and publish first message
    target_.position_pub = node_->createTargetPositionPublisher(target_id_);
    target_.position_pub->publish(target_.position);

    // Set update timer
    time_elapsed_ = 0.0f;
    last_update_time_ = node_->now();
    update_timer_ = node_->createTimer(update_rate_, std::bind(&TargetState::update, this));
}

void TargetState::onModuleShutdown()
{
    // Destroy target publisher
    target_.position_pub.reset();
    // Destroy update timer
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE: Update motion
// ════════════════════════════════════════════════════════════════════════════

void TargetState::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        RCLCPP_INFO(node_->get_logger(), "Target state: Skipping update due to invalid status");
        return;
    }

    // Compute time step
    auto current_time = node_->now();
    float dt = (current_time - last_update_time_).seconds();
    last_update_time_ = current_time;

    // Limit dt to prevent extreme values after pauses
    dt = std::min(dt, cmd_timeout_);

    // Update time elapsed
    time_elapsed_ += dt;

    // Get next and previous points
    TrajectoryParser::Point next_point;
    TrajectoryParser::Point prev_point;
    if (trajectory_.current_idx + 1 >= trajectory_.num_points)
        next_point = trajectory_.points[trajectory_.current_idx];
    else
        next_point = trajectory_.points[trajectory_.current_idx + 1];
    if (trajectory_.current_idx - 1 < 0)
        prev_point = trajectory_.points[trajectory_.current_idx];
    else
        prev_point = trajectory_.points[trajectory_.current_idx - 1];

    // Find the closest trajectory point based on time and direction
    if (!trajectory_.reverse)
    {
        if (trajectory_.current_idx < trajectory_.num_points - 1 && next_point.t <= time_elapsed_)
        {
            trajectory_.current_idx++;
            // If we reach the end, start moving backwards
            if (trajectory_.current_idx == trajectory_.num_points - 1)
            {
                trajectory_.reverse = true;
            }
        }
    }
    else
    {
        if (trajectory_.current_idx > 0 && prev_point.t <= time_elapsed_)
        {
            trajectory_.current_idx--;
            // If we reach the start, start moving forwards again
            if (trajectory_.current_idx == 0)
            {
                trajectory_.reverse = false;
            }
        }
    }

    // Update and publish target position
    TrajectoryParser::Point pos = trajectory_.points[trajectory_.current_idx];
    target_.position.header.stamp = node_->now();
    target_.position.point.x = pos.x;
    target_.position.point.y = pos.y;
    target_.position.point.z = pos.z;
    target_.position_pub->publish(target_.position);
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool TargetState::checkStatus()
{
    // Check 1: Trajectory must have valid points
    if (trajectory_.num_points == 0)
    {
        RCLCPP_INFO(node_->get_logger(), "Target state: No trajectory points available for target %s", target_id_.c_str());
        return false;
    }

    // All checks passed
    return true;
}