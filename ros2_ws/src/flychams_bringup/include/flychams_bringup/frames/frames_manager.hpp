#pragma once

// Base module include
#include "flychams_core/base/base_module.hpp"

// ROS2 includes
#include "geographic_msgs/msg/geo_point.hpp"
#include "mavros_msgs/msg/home_position.hpp"

namespace flychams::bringup
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Class for managing frames for a single agent
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-12-07
	 * ════════════════════════════════════════════════════════════════
	 */
	class FramesManager : public core::BaseModule
	{
	public: // Constructor/Destructor
		FramesManager(const core::ID& agent_id, core::NodePtr node, core::ConfigTools::SharedPtr config_tools, core::TopicTools::SharedPtr topic_tools, core::TransformTools::SharedPtr transform_tools, core::CallbackGroupPtr module_cb_group)
			: BaseModule(node, config_tools, topic_tools, transform_tools, module_cb_group), agent_id_(agent_id)
		{
			init();
		}

	protected: // Overrides
		void onInit() override;
		void onShutdown() override;

	public: // Types
		using SharedPtr = std::shared_ptr<FramesManager>;

	private: // Parameters
		core::ID agent_id_;

	public: // Frames management
		void setupCameraFrames();
	};

} // namespace flychams::bringup
