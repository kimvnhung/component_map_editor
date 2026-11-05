#ifndef COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_H
#define COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_H

#include "component_map_runtime/ComponentContract.h"
#include "component_map_runtime/ComponentDiscoveryTypes.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QString>

namespace cme::runtime
{

class ComponentCatalog;
class NetworkComponentSource;
class RuntimePersistence;

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

class ComponentDiscovery
{
public:
    explicit ComponentDiscovery(ComponentCatalog *catalog = nullptr)
        : m_catalog(catalog)
    {
    }

    void setCatalog(ComponentCatalog *catalog) { m_catalog = catalog; }
    void setPersistence(RuntimePersistence *persistence) { m_persistence = persistence; }
    bool addSource(NetworkComponentSource *source, QString *error = nullptr);
    bool removeSource(const QString &sourceId);
    QList<DiscoveryRefreshResult> refreshAll();
    DiscoveryRefreshResult refreshSource(const QString &sourceId);
    DiscoverySourceState sourceState(const QString &sourceId) const;
    QStringList sourceIds() const;

private:
    struct SourceEntry
    {
        NetworkComponentSource *provider = nullptr;
        DiscoverySourceState state;
    };

    DiscoveryRefreshResult refreshEntry(const QString &sourceId, SourceEntry &entry) const;
    static void appendDiagnostic(DiscoveryRefreshResult &result, const QString &code,
                                 const QString &message, bool retryable = false);

    mutable QMutex m_mutex;
    QHash<QString, SourceEntry> m_sources;
    ComponentCatalog *m_catalog = nullptr; // non-owning runtime catalog
    RuntimePersistence *m_persistence = nullptr; // non-owning runtime persistence

};

QString discoverySourceKindName(DiscoverySourceKind kind);
QString discoveryHealthName(DiscoveryHealth health);

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_H
