#include "TokenKeyFeatureFlags.h"

#include <atomic>

namespace {

constexpr bool kDefaultConnectionTokenKeyEnabled = false;
constexpr bool kDefaultInspectorTokenKeySelectorEnabled = false;

std::atomic_bool g_connectionTokenKeyEnabled{kDefaultConnectionTokenKeyEnabled};
std::atomic_bool g_inspectorTokenKeySelectorEnabled{kDefaultInspectorTokenKeySelectorEnabled};

} // namespace

namespace cme::tokenkey {

bool FeatureFlags::connectionTokenKeyEnabled()
{
    return g_connectionTokenKeyEnabled.load(std::memory_order_relaxed);
}

void FeatureFlags::setConnectionTokenKeyEnabled(bool enabled)
{
    g_connectionTokenKeyEnabled.store(enabled, std::memory_order_relaxed);
}

bool FeatureFlags::inspectorTokenKeySelectorEnabled()
{
    return g_inspectorTokenKeySelectorEnabled.load(std::memory_order_relaxed);
}

void FeatureFlags::setInspectorTokenKeySelectorEnabled(bool enabled)
{
    g_inspectorTokenKeySelectorEnabled.store(enabled, std::memory_order_relaxed);
}

void FeatureFlags::resetDefaults()
{
    setConnectionTokenKeyEnabled(kDefaultConnectionTokenKeyEnabled);
    setInspectorTokenKeySelectorEnabled(kDefaultInspectorTokenKeySelectorEnabled);
}

} // namespace cme::tokenkey
