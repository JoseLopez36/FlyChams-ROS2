#pragma once

// Base node include
#include "flychams_common/base/base_discoverer_node.hpp"

namespace flychams::core
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base module for sub-nodes extending BaseDiscovererNode
     *
     * @details
     * Provides access to BaseDiscovererNode functionalities.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-15
     * ════════════════════════════════════════════════════════════════
     */
    class BaseDiscovererModule
    {
    public: // Constructor/Destructor
        BaseDiscovererModule(BaseDiscovererNode::SharedPtr node);

        void init();

        virtual ~BaseDiscovererModule();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseDiscovererModule>;

    protected: // Overridable methods
        virtual void onModuleInit() {}
        virtual void onModuleShutdown() {}

    protected: // Node access
        BaseDiscovererNode::SharedPtr node_;
    };

} // namespace flychams::core