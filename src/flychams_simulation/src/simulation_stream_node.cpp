#include "rclcpp/rclcpp.hpp"

// Module include
#include "flychams_simulation/stream/simulation_stream.hpp"

// Base node include
#include "flychams_common/base/base_node.hpp"

using namespace flychams::common;

using namespace flychams::simulation;

/**
 * ════════════════════════════════════════════════════════════════
 * @brief Simulation stream node for bridging AirSim RTSP view
 * streams (SCENARIOCAM, AGENTCAM, PAYLOADCAM) to ROS2 image topics
 * ════════════════════════════════════════════════════════════════
 * @author Jose Francisco Lopez Ruiz
 * @date 2026-05-21
 * ════════════════════════════════════════════════════════════════
 */
class SimulationStreamNode : public BaseNode
{
public: // Constructor/Destructor
    SimulationStreamNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : BaseNode(node_name, options)
    {
        // Nothing to do
    }

    void onNodeInit() override
    {
        // Create simulation stream module
        simulation_stream_ = std::make_shared<SimulationStream>(sharedFromThis());

        RCLCPP_INFO(node_->get_logger(), "Simulation stream node initialized");
    }

    void onNodeShutdown() override
    {
        // Destroy simulation stream module
        simulation_stream_.reset();
    }

private: // Components
    SimulationStream::SharedPtr simulation_stream_;
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
    auto node = std::make_shared<SimulationStreamNode>("simulation_stream_node", options);
    node->init();
    // Create executor and add node
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    // Spin node
    executor.spin();
    // Shutdown
    rclcpp::shutdown();
    return 0;
}