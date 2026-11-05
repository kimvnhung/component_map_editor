#include "component_map_runtime/ComponentDiscovery.h"

namespace cme::runtime
{

bool DiscoveryRetryPolicy::isValid() const
{
    return maxAttempts > 0 && initialBackoffMs >= 0 && maxBackoffMs >= initialBackoffMs;
}

bool DiscoverySourceConfig::isValid() const
{
    if (sourceId.trimmed().isEmpty() || timeoutMs <= 0 || !retryPolicy.isValid())
        return false;
    if (kind == DiscoverySourceKind::Network)
        return !endpoint.trimmed().isEmpty();
    return endpoint.trimmed().isEmpty();
}

bool DiscoveryRefreshResult::hasUsableDescriptors() const
{
    return success && !descriptors.isEmpty();
}

QString discoverySourceKindName(DiscoverySourceKind kind)
{
    return kind == DiscoverySourceKind::Network ? QStringLiteral("network")
                                                 : QStringLiteral("builtin");
}

QString discoveryHealthName(DiscoveryHealth health)
{
    switch (health)
    {
    case DiscoveryHealth::Unknown: return QStringLiteral("unknown");
    case DiscoveryHealth::Healthy: return QStringLiteral("healthy");
    case DiscoveryHealth::Degraded: return QStringLiteral("degraded");
    case DiscoveryHealth::Timeout: return QStringLiteral("timeout");
    case DiscoveryHealth::Unavailable: return QStringLiteral("unavailable");
    }
    return QStringLiteral("unknown");
}

} // namespace cme::runtime
