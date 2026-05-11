#include "rclcpp/rclcpp.hpp"

// Simulation includes
#include "flychams_simulation/gui/simulation_gui.hpp"

// Core includes
#include "flychams_common/base/base_discoverer_node.hpp"

using namespace flychams::core;
using namespace flychams::simulation;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief GUI manager node for the FlyingChameleons simulated system
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2025-03-01
 * ════════════════════════════════════════════════════════════════
 */
class SimulationGuiNode : public BaseDiscovererNode
{
public: // Constructor/Destructor
    SimulationGuiNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseDiscovererNode(node_name, options)
    {
        // Nothing to do
    }

    void onInit() override
    {
        // Initialize selected agent
        selected_agent_id_ = "NONE";

        // Initialize Simulation GUIs
        simulation_guis_.clear();
    }

    void onShutdown() override
    {
        // Destroy Simulation GUIs
        simulation_guis_.clear();
    }

private: // Agent management
    void onAgentSelected()
    {
        if (selected_agent_id_ != "NONE")
        {
            // Start selected simulation GUI and stop the rest
            for (const auto& [id, gui] : simulation_guis_)
            {
                if (id == selected_agent_id_)
                {
                    RCLCPP_INFO(node_->get_logger(), "Starting Simulation GUI for agent %s", selected_agent_id_.c_str());
                    gui->start();
                }
                else
                {
                    gui->stop();
                }
            }
        }
    }

    void onAddAgent(const ID& agent_id) override
    {
        // Use callback group from discovery node (to avoid race conditions)
        // Create and add Simulation GUI
        auto gui = std::make_shared<SimulationGui>(agent_id, node_, settings_tools_, topic_tools_, transform_tools_, discovery_cb_group_);
        simulation_guis_.insert({ agent_id, gui });

        RCLCPP_INFO(node_->get_logger(), "Simulation GUI created for agent %s", agent_id.c_str());

        // Select agent if no agent is selected
        if (selected_agent_id_ == "NONE")
        {
            selected_agent_id_ = agent_id;
            onAgentSelected();
        }
    }

    void onRemoveAgent(const ID& agent_id) override
    {
        // Remove agent from Simulation GUI
        simulation_guis_.erase(agent_id);

        // Select new agent if it is the one being removed
        if (selected_agent_id_ == agent_id)
        {
            // Get first agent if available
            if (!simulation_guis_.empty())
            {
                auto it = simulation_guis_.begin();
                selected_agent_id_ = it->first;
                onAgentSelected();
            }
            else
            {
                // No agents left, reset selection
                selected_agent_id_ = "NONE";
            }
        }
    }

    void onAddTarget(const ID& target_id) override
    {
        // Targets are not handled by this node
    }

    void onRemoveTarget(const ID& target_id) override
    {
        // Targets are not handled by this node
    }

    void onAddCluster(const ID& cluster_id) override
    {
        // Clusters are not handled by this node
    }

    void onRemoveCluster(const ID& cluster_id) override
    {
        // Clusters are not handled by this node
    }

private: // Components
    // Selected agent
    ID selected_agent_id_;
    // Simulation GUI per agent
    std::unordered_map<ID, SimulationGui::SharedPtr> simulation_guis_;
};

int main(int argc, char** argv)
{
    // Initialize ROS
    rclcpp::init(argc, argv);
    // Create node options
    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(true);
    // Create node
    auto node = std::make_shared<SimulationGuiNode>("simulation_gui_node", options);
    node->init();
    // Spin node
    rclcpp::spin(node);
    // Shutdown
    rclcpp::shutdown();
    return 0;
}