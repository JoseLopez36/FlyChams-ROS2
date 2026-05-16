#pragma once

// Base node include
#include "flychams_common/base/base_status_discoverer_node.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base module for sub-nodes extending BaseStatusDiscovererNode
     *
     * @details
     * Provides access to BaseStatusDiscovererNode functionalities.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-15
     * ════════════════════════════════════════════════════════════════
     */
    class BaseStatusDiscovererModule
    {
    public: // Constructor/Destructor
        BaseStatusDiscovererModule(BaseStatusDiscovererNode::SharedPtr node);

        void init();

        virtual ~BaseStatusDiscovererModule();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseStatusDiscovererModule>;

    protected: // Overridable methods
        virtual void onModuleInit() {}
        virtual void onModuleShutdown() {}

    protected: // Node access
        BaseStatusDiscovererNode::SharedPtr node_;
    };

} // namespace flychams::common