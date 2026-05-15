#include "flychams_coordinator/registration/element_registration.hpp"

using namespace flychams::core;
using namespace flychams::coordinator;

// ════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR: Constructor and destructor
// ════════════════════════════════════════════════════════════════════════════

void ElementRegistration::onInit()
{
	// Create registration instances for each element type
	agent_registration_ = std::make_shared<AgentRegistration>(node_, settings_tools_, topic_tools_, transform_tools_, module_cb_group_);
	target_registration_ = std::make_shared<TargetRegistration>(node_, settings_tools_, topic_tools_, transform_tools_, module_cb_group_);
	cluster_registration_ = std::make_shared<ClusterRegistration>(node_, settings_tools_, topic_tools_, transform_tools_, module_cb_group_);

	// Get all elements
	agents_ = agent_registration_->getAgents();
	targets_ = target_registration_->getTargets();
	clusters_ = cluster_registration_->getClusters();

	// Check if every element type is correctly registered
	if (agents_.empty())
	{
		RCLCPP_ERROR(node_->get_logger(), "No agents registered. Cannot setup the simulation");
		rclcpp::shutdown();
		return;
	}
	if (targets_.empty())
	{
		RCLCPP_ERROR(node_->get_logger(), "No targets registered. Cannot setup the simulation");
		rclcpp::shutdown();
		return;
	}
	if (clusters_.empty())
	{
		RCLCPP_ERROR(node_->get_logger(), "No clusters registered. Cannot setup the simulation");
		rclcpp::shutdown();
		return;
	}

	// Register all agents, targets and clusters
	for (const auto& agent_id : agents_)
		registerElement(agent_id, ElementType::Agent);
	for (const auto& target_id : targets_)
		registerElement(target_id, ElementType::Target);
	for (const auto& cluster_id : clusters_)
		registerElement(cluster_id, ElementType::Cluster);

	// Create registration publisher
	registration_pub_ = topic_tools_->createRegistrationPublisher();

	// Create global origin publisher
	global_origin_pub_ = topic_tools_->createGlobalOriginPublisher();

	// Publish global origin
	GeoPointStampedMsg origin_msg;
	origin_msg.header = RosUtils::createHeader(node_, transform_tools_->getGlobalFrame());
	origin_msg.position.latitude = settings_tools_->getEnvironment().geopoint.latitude;
	origin_msg.position.longitude = settings_tools_->getEnvironment().geopoint.longitude;
	origin_msg.position.altitude = settings_tools_->getEnvironment().geopoint.altitude;
	global_origin_pub_->publish(origin_msg);

	// Initialize registration update timer (1 Hz)
	update_timer_ = node_->create_wall_timer(
		std::chrono::duration<float>(1.0f),
		std::bind(&ElementRegistration::publishRegistration, this),
		module_cb_group_);
}

void ElementRegistration::onShutdown()
{
	// Unregister all elements
	for (const auto& agent_id : agents_)
		unregisterElement(agent_id, ElementType::Agent);
	for (const auto& target_id : targets_)
		unregisterElement(target_id, ElementType::Target);
	for (const auto& cluster_id : clusters_)
		unregisterElement(cluster_id, ElementType::Cluster);
	// Destroy timer
	update_timer_.reset();
	// Destroy publishers
	registration_pub_.reset();
	global_origin_pub_.reset();
	// Destroy registration instances
	agent_registration_.reset();
	target_registration_.reset();
	cluster_registration_.reset();
	// Clear elements
	agents_.clear();
	targets_.clear();
	clusters_.clear();
}

// ════════════════════════════════════════════════════════════════════════════
// REGISTRATION METHODS
// ════════════════════════════════════════════════════════════════════════════

void ElementRegistration::registerElement(const ID& element_id, const ElementType& element_type)
{
	if (registered_elements_.find(element_id) != registered_elements_.end())
		return;
	registered_elements_.insert({ element_id, element_type });
	RCLCPP_INFO(node_->get_logger(), "Element %s registered", element_id.c_str());
}

void ElementRegistration::unregisterElement(const ID& element_id, const ElementType& element_type)
{
	if (registered_elements_.find(element_id) == registered_elements_.end())
		return;
	registered_elements_.erase(element_id);
	RCLCPP_INFO(node_->get_logger(), "Element %s unregistered", element_id.c_str());
}

void ElementRegistration::publishRegistration()
{
	RegistrationMsg msg;
	for (const auto& [id, type] : registered_elements_)
	{
		ElementMsg element;
		element.id = id;
		element.type = static_cast<uint8_t>(type);
		msg.elements.push_back(element);
	}
	registration_pub_->publish(msg);
}