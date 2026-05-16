#pragma once

// Base node include
#include "flychams_common/base/base_status_node.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base module for sub-nodes extending BaseStatusNode
     *
     * @details
     * Provides access to BaseStatusNode functionalities.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-15
     * ════════════════════════════════════════════════════════════════
     */
    class BaseStatusModule
    {
    public: // Constructor/Destructor
        BaseStatusModule(BaseStatusNode::SharedPtr node);

        void init();

        virtual ~BaseStatusModule();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseStatusModule>;

    protected: // Overridable methods
        virtual void onModuleInit() {}
        virtual void onModuleShutdown() {}

    protected: // Node access
        BaseStatusNode::SharedPtr node_;
    };

} // namespace flychams::common