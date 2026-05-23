#include "flychams_operator/markers/markers_generator.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void MarkersGenerator::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 10.0f);

    // Publisher
    scene_pub_ = node_->createScenePublisher();

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&MarkersGenerator::update, this));
}

void MarkersGenerator::onModuleShutdown()
{
    update_timer_.reset();
    scene_pub_.reset();
    agent_markers_.clear();
    target_markers_.clear();
    cluster_markers_.clear();
}

// ════════════════════════════════════════════════════════════════════════════
// ELEMENT MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void MarkersGenerator::addAgent(const ID& agent_id)
{
    agent_markers_.emplace(agent_id, std::make_shared<AgentMarkers>(agent_id, node_));
    cluster_markers_.emplace(agent_id, std::make_shared<ClusterMarkers>(agent_id, node_));
}

void MarkersGenerator::removeAgent(const ID& agent_id)
{
    agent_markers_.erase(agent_id);
    cluster_markers_.erase(agent_id);
}

void MarkersGenerator::addTarget(const ID& target_id)
{
    target_markers_.emplace(target_id, std::make_shared<TargetMarkers>(target_id, node_));
}

void MarkersGenerator::removeTarget(const ID& target_id)
{
    target_markers_.erase(target_id);
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void MarkersGenerator::update()
{
    FoxSceneUpdateMsg scene_msg;

    // Collect entities from each agent generator
    for (auto& [id, gen] : agent_markers_)
    {
        gen->getEntities(scene_msg);
    }

    // Collect entities from each target generator
    for (auto& [id, gen] : target_markers_)
    {
        gen->getEntities(scene_msg);
    }

    // Collect entities from each cluster generator
    for (auto& [id, gen] : cluster_markers_)
    {
        gen->getEntities(scene_msg);
    }

    // Publish the combined scene
    scene_pub_->publish(scene_msg);
}