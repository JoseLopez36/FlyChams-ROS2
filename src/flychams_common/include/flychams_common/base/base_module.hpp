#pragma once

// Base node include
#include "flychams_common/base/base_node.hpp"

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Base module for sub-nodes extending BaseNode
     *
     * @details
     * Provides access to BaseNode functionalities.
     *
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2025-05-15
     * ════════════════════════════════════════════════════════════════
     */
    class BaseModule
    {
    public: // Constructor/Destructor
        BaseModule(BaseNode::SharedPtr node);

        void init();

        virtual ~BaseModule();

        void shutdown();

    public: // Types
        using SharedPtr = std::shared_ptr<BaseModule>;

    protected: // Overridable methods
        virtual void onModuleInit() {}
        virtual void onModuleShutdown() {}

    protected: // Node access
        BaseNode::SharedPtr node_;
    };

} // namespace flychams::common