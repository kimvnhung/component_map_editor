#include "ExecutionMigrationFlags.h"

#include <atomic>

namespace {

std::atomic_bool g_tokenTransportEnabled{false};

} // namespace

namespace cme::execution {

bool MigrationFlags::tokenTransportEnabled()
{
    return g_tokenTransportEnabled.load(std::memory_order_relaxed);
}

void MigrationFlags::setTokenTransportEnabled(bool enabled)
{
    g_tokenTransportEnabled.store(enabled, std::memory_order_relaxed);
}

void MigrationFlags::resetDefaults()
{
    setTokenTransportEnabled(false);
}

} // namespace cme::execution
