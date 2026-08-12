#ifndef PROVIDERADAPTER_H
#define PROVIDERADAPTER_H

#include "ExecutionStateCapture.h"

enum class ThreadAffinity { THREAD_SAFE, MAIN_THREAD };

class ProviderAdapter
{
    virtual ExecuteResult execute(const ExecutionContext& ctx) = 0;
    virtual ThreadAffinity affinity() const = 0;
};

class MainThreadProviderAdapter : public ProviderAdapter
{
    // Marshal execution to main thread via QMetaObject::invokeMethod
    ExecuteResult execute(const ExecutionContext& ctx) override
    {
        // Implementation to marshal execution to main thread
        return ExecuteResult{true, {}, "Executed on main thread"};
    }
};

class DirectProviderAdapter : public ProviderAdapter
{
    // Call directly (THREAD_SAFE)
    ExecuteResult execute(const ExecutionContext& ctx) override
    {
        // Direct execution
        return ExecuteResult{true, {}, "Executed directly"};
    }
};

#endif // PROVIDERADAPTER_H
