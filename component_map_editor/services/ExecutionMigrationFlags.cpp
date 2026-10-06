#include "ExecutionMigrationFlags.h"

#include <atomic>

namespace {

constexpr bool kDefaultTokenTransportEnabled = true;
constexpr bool kCompatibilityWindowOpen = true;
constexpr bool kDefaultActorEngineEnabled = false;

std::atomic_bool g_tokenTransportEnabled{kDefaultTokenTransportEnabled};
std::atomic_bool g_actorEngineEnabled{kDefaultActorEngineEnabled};

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
    setActorEngineEnabled(kDefaultActorEngineEnabled);
}

bool MigrationFlags::compatibilityWindowOpen()
{
    return kCompatibilityWindowOpen;
}

bool MigrationFlags::actorEngineEnabled()
{
    return g_actorEngineEnabled.load(std::memory_order_relaxed);
}

void MigrationFlags::setActorEngineEnabled(bool enabled)
{
    g_actorEngineEnabled.store(enabled, std::memory_order_relaxed);
}

} // namespace cme::execution
