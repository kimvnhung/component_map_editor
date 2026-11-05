#ifndef COMPONENT_MAP_RUNTIME_COMPONENT_CATALOG_H
#define COMPONENT_MAP_RUNTIME_COMPONENT_CATALOG_H

#include "component_map_runtime/ComponentDiscoveryTypes.h"

#include <QList>
#include <QReadWriteLock>

namespace cme::runtime
{

struct CatalogFilter
{
    QString identity;
    QString sourceId;
    QString category;
    ComponentSource source = ComponentSource::Builtin;
    bool filterSource = false;
    ComponentStatus status = ComponentStatus::Available;
    bool filterStatus = false;
};

class ComponentCatalog
{
public:
    bool registerDescriptor(const ComponentDescriptor &descriptor,
                            ComponentDiagnostic *diagnostic = nullptr);
    bool refreshDescriptor(const ComponentDescriptor &descriptor,
                           ComponentDiagnostic *diagnostic = nullptr);
    bool transitionStatus(const ComponentKey &key, ComponentStatus status,
                          ComponentDiagnostic *diagnostic = nullptr);
    bool remove(const ComponentKey &key, ComponentDiagnostic *diagnostic = nullptr);

    QList<ComponentDescriptor> snapshot(const CatalogFilter &filter = {}) const;
    ComponentDescriptor find(const QString &identity, const QString &version,
                             const QString &sourceId = {}) const;
    QList<ComponentDiagnostic> diagnostics(const ComponentKey &key) const;
    quint64 snapshotVersion() const;

private:
    struct Entry
    {
        ComponentDescriptor descriptor;
        QList<ComponentDiagnostic> diagnostics;
    };

    static void setDiagnostic(ComponentDiagnostic *target, const QString &code,
                              const QString &message, const QString &fieldPath = {});

    mutable QReadWriteLock m_lock;
    QHash<ComponentKey, Entry> m_entries;
    quint64 m_snapshotVersion = 0;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_COMPONENT_CATALOG_H
