#ifndef COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_H
#define COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_H

#include "component_map_runtime/ComponentContract.h"
#include "component_map_runtime/ComponentDiscoveryTypes.h"

#include <QDateTime>
#include <QList>
#include <QString>

namespace cme::runtime
{

enum class DiscoverySourceKind
{
    Builtin,
    Network
};

enum class DiscoveryHealth
{
    Unknown,
    Healthy,
    Degraded,
    Timeout,
    Unavailable
};

struct DiscoveryRetryPolicy
{
    int maxAttempts = 1;
    int initialBackoffMs = 0;
    int maxBackoffMs = 0;

    bool isValid() const;
};

struct DiscoverySourceConfig
{
    QString sourceId;
    DiscoverySourceKind kind = DiscoverySourceKind::Builtin;
    QString endpoint;
    int timeoutMs = 5000;
    DiscoveryRetryPolicy retryPolicy;
    bool trusted = false;

    bool isValid() const;
};

struct DiscoverySourceState
{
    DiscoveryHealth health = DiscoveryHealth::Unknown;
    QDateTime lastRefresh;
    QStringList diagnostics;
};

struct DiscoveryRefreshResult
{
    QString sourceId;
    DiscoveryHealth health = DiscoveryHealth::Unknown;
    QList<ComponentDescriptor> descriptors;
    QList<ComponentDiagnostic> diagnostics;
    bool success = false;

    bool hasUsableDescriptors() const;
};

QString discoverySourceKindName(DiscoverySourceKind kind);
QString discoveryHealthName(DiscoveryHealth health);

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_H
