#include "component_map_runtime/ComponentDiscovery.h"

#include "component_map_runtime/NetworkComponentSource.h"
#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/RuntimePersistence.h"

#include <QMutexLocker>
#include <QDateTime>

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

bool ComponentDiscovery::addSource(NetworkComponentSource *source, QString *error)
{
    if (!source)
    {
        if (error) *error = QStringLiteral("Discovery source must not be null.");
        return false;
    }

    const QString id = source->sourceId().trimmed();
    const DiscoverySourceConfig config = source->configuration();
    if (id.isEmpty() || id != config.sourceId || !config.isValid())
    {
        if (error) *error = QStringLiteral("Discovery source configuration is invalid.");
        return false;
    }

    QMutexLocker locker(&m_mutex);
    if (m_sources.contains(id))
    {
        if (error) *error = QStringLiteral("Discovery source is already registered: %1").arg(id);
        return false;
    }
    m_sources.insert(id, SourceEntry{source, {}});
    return true;
}

bool ComponentDiscovery::removeSource(const QString &sourceId)
{
    QMutexLocker locker(&m_mutex);
    return m_sources.remove(sourceId.trimmed()) > 0;
}

QList<DiscoveryRefreshResult> ComponentDiscovery::refreshAll()
{
    QStringList ids;
    {
        QMutexLocker locker(&m_mutex);
        ids = m_sources.keys();
    }
    std::sort(ids.begin(), ids.end());

    QList<DiscoveryRefreshResult> results;
    for (const QString &id : ids)
        results.append(refreshSource(id));
    return results;
}

DiscoveryRefreshResult ComponentDiscovery::refreshSource(const QString &sourceId)
{
    const QString id = sourceId.trimmed();
    QMutexLocker locker(&m_mutex);
    auto it = m_sources.find(id);
    if (it == m_sources.end())
    {
        DiscoveryRefreshResult result;
        result.sourceId = id;
        result.health = DiscoveryHealth::Unavailable;
        appendDiagnostic(result, QStringLiteral("source_not_found"),
                         QStringLiteral("Discovery source is not registered."));
        return result;
    }

    SourceEntry &entry = it.value();
    DiscoveryRefreshResult result = refreshEntry(id, entry);
    entry.state.health = result.health;
    entry.state.lastRefresh = QDateTime::currentDateTimeUtc();
    entry.state.diagnostics.clear();
    for (const ComponentDiagnostic &diagnostic : result.diagnostics)
        entry.state.diagnostics.append(diagnostic.code + QStringLiteral(": ") + diagnostic.message);
    return result;
}

DiscoverySourceState ComponentDiscovery::sourceState(const QString &sourceId) const
{
    QMutexLocker locker(&m_mutex);
    return m_sources.value(sourceId.trimmed()).state;
}

QStringList ComponentDiscovery::sourceIds() const
{
    QMutexLocker locker(&m_mutex);
    QStringList ids = m_sources.keys();
    std::sort(ids.begin(), ids.end());
    return ids;
}

DiscoveryRefreshResult ComponentDiscovery::refreshEntry(const QString &sourceId,
                                                        SourceEntry &entry) const
{
    DiscoveryRefreshResult result;
    result.sourceId = sourceId;
    const DiscoverySourceConfig config = entry.provider->configuration();

    auto persistEvent = [this](const QString &eventId,
                                         const ComponentKey &component,
                                         DiscoveryEvent::Kind kind,
                                         ComponentStatus status,
                                         const QString &contractVersion,
                                         const QList<ComponentDiagnostic> &diagnostics = {}) {
        if (!m_persistence)
            return;
        DiscoveryEvent event;
        event.eventId = eventId;
        event.timestamp = QDateTime::currentDateTimeUtc();
        event.component = component;
        event.kind = kind;
        event.status = status;
        event.diagnostics = diagnostics;
        QString ignored;
        m_persistence->recordDiscoveryLifecycleEvent(event, contractVersion, &ignored);
    };

    if (!config.trusted)
    {
        result.health = DiscoveryHealth::Unavailable;
        appendDiagnostic(result, QStringLiteral("untrusted_source"),
                         QStringLiteral("Network discovery is deny-by-default until trusted."));
        for (const auto &diagnostic : result.diagnostics)
            persistEvent(QStringLiteral("%1:unavailable").arg(sourceId),
                         {QStringLiteral("*"), sourceId, {}}, DiscoveryEvent::Kind::Unavailable,
                         ComponentStatus::Unavailable, {}, {diagnostic});
        return result;
    }

    NetworkDiscoveryResult discovery;
    const int attempts = qMax(1, config.retryPolicy.maxAttempts);
    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        discovery = entry.provider->discover();
        if (discovery.success)
            break;
    }

    result.descriptors = discovery.descriptors;
    result.diagnostics = validateNetworkDiscoveryResult(discovery, config);
    result.success = discovery.success && result.diagnostics.isEmpty();
    result.health = result.success ? DiscoveryHealth::Healthy : DiscoveryHealth::Unavailable;
    if (!discovery.success && result.diagnostics.isEmpty())
        appendDiagnostic(result, QStringLiteral("discovery_failed"),
                         QStringLiteral("Discovery provider failed after retry attempts."), true);

    if (!result.success)
    {
        for (const auto &diagnostic : result.diagnostics)
        {
            persistEvent(QStringLiteral("%1:%2").arg(sourceId, diagnostic.code),
                         {QStringLiteral("*"), sourceId, {}}, DiscoveryEvent::Kind::Rejected,
                         ComponentStatus::Rejected, {}, {diagnostic});
        }
    }

    if (result.success && m_catalog)
    {
        for (const ComponentDescriptor &descriptor : result.descriptors)
        {
            const bool existed = !m_catalog->find(descriptor.identity, descriptor.version,
                                                   descriptor.sourceId).identity.isEmpty();
            ComponentDiagnostic diagnostic;
            if (!m_catalog->refreshDescriptor(descriptor, &diagnostic))
            {
                result.success = false;
                result.health = DiscoveryHealth::Degraded;
                result.diagnostics.append(diagnostic);
                persistEvent(QStringLiteral("%1:%2:rejected").arg(sourceId, descriptor.identity),
                             componentKey(descriptor), DiscoveryEvent::Kind::Rejected,
                             ComponentStatus::Rejected, descriptor.contractVersion, {diagnostic});
                continue;
            }

            persistEvent(QStringLiteral("%1:%2:%3").arg(sourceId, descriptor.identity,
                                                        existed ? QStringLiteral("refreshed")
                                                                : QStringLiteral("registered")),
                         componentKey(descriptor),
                         existed ? DiscoveryEvent::Kind::Refreshed
                                 : DiscoveryEvent::Kind::Registered,
                         descriptor.status, descriptor.contractVersion);
        }
    }
    return result;
}

void ComponentDiscovery::appendDiagnostic(DiscoveryRefreshResult &result,
                                          const QString &code,
                                          const QString &message,
                                          bool retryable)
{
    result.diagnostics.append(ComponentDiagnostic{code, message, {}, retryable});
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
