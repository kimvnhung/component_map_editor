#include "ExecutionMigrationFlags.h"

#include <atomic>

namespace {

constexpr bool kDefaultTokenTransportEnabled = true;
constexpr bool kCompatibilityWindowOpen = true;

std::atomic_bool g_tokenTransportEnabled{kDefaultTokenTransportEnabled};

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
    setTokenTransportEnabled(kDefaultTokenTransportEnabled);
}

bool MigrationFlags::compatibilityWindowOpen()
{
    return kCompatibilityWindowOpen;
}

} // namespace cme::execution
