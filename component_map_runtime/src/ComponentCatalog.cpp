#include "component_map_runtime/ComponentCatalog.h"

#include <algorithm>

namespace cme::runtime
{

void ComponentCatalog::setDiagnostic(ComponentDiagnostic *target, const QString &code,
                                     const QString &message, const QString &fieldPath)
{
    if (!target)
        return;
    target->code = code;
    target->message = message;
    target->fieldPath = fieldPath;
}

bool ComponentCatalog::registerDescriptor(const ComponentDescriptor &descriptor,
                                          ComponentDiagnostic *diagnostic)
{
    const QStringList errors = validateComponentDescriptor(descriptor);
    if (!errors.isEmpty())
    {
        setDiagnostic(diagnostic, QStringLiteral("invalid_descriptor"), errors.constFirst());
        return false;
    }

    const ComponentKey key = componentKey(descriptor);
    QWriteLocker locker(&m_lock);
    const auto existing = m_entries.constFind(key);
    if (existing != m_entries.cend())
    {
        if (equivalentComponentDescriptor(existing->descriptor, descriptor))
        {
            setDiagnostic(diagnostic, QStringLiteral("already_registered"),
                          QStringLiteral("Equivalent descriptor is already registered."));
        }
        else
        {
            setDiagnostic(diagnostic, QStringLiteral("descriptor_conflict"),
                          QStringLiteral("A different descriptor uses the same identity, source, and version."));
        }
        return false;
    }

    Entry entry;
    entry.descriptor = descriptor;
    m_entries.insert(key, std::move(entry));
    ++m_snapshotVersion;
    return true;
}

bool ComponentCatalog::refreshDescriptor(const ComponentDescriptor &descriptor,
                                         ComponentDiagnostic *diagnostic)
{
    const QStringList errors = validateComponentDescriptor(descriptor);
    if (!errors.isEmpty())
    {
        setDiagnostic(diagnostic, QStringLiteral("invalid_descriptor"), errors.constFirst());
        return false;
    }

    const ComponentKey key = componentKey(descriptor);
    QWriteLocker locker(&m_lock);
    const auto existing = m_entries.find(key);
    if (existing == m_entries.end())
    {
        Entry entry;
        entry.descriptor = descriptor;
        m_entries.insert(key, std::move(entry));
    }
    else
    {
        existing->descriptor = descriptor;
    }
    ++m_snapshotVersion;
    return true;
}

bool ComponentCatalog::transitionStatus(const ComponentKey &key, ComponentStatus status,
                                        ComponentDiagnostic *diagnostic)
{
    QWriteLocker locker(&m_lock);
    auto entry = m_entries.find(key);
    if (entry == m_entries.end())
    {
        setDiagnostic(diagnostic, QStringLiteral("component_not_found"),
                      QStringLiteral("The requested component is not in the catalog."));
        return false;
    }
    entry->descriptor.status = status;
    ++m_snapshotVersion;
    return true;
}

bool ComponentCatalog::remove(const ComponentKey &key, ComponentDiagnostic *diagnostic)
{
    QWriteLocker locker(&m_lock);
    if (m_entries.remove(key) == 0)
    {
        setDiagnostic(diagnostic, QStringLiteral("component_not_found"),
                      QStringLiteral("The requested component is not in the catalog."));
        return false;
    }
    ++m_snapshotVersion;
    return true;
}

QList<ComponentDescriptor> ComponentCatalog::snapshot(const CatalogFilter &filter) const
{
    QReadLocker locker(&m_lock);
    QList<ComponentDescriptor> result;
    for (const auto &entry : m_entries)
    {
        const auto &descriptor = entry.descriptor;
        if (!filter.identity.isEmpty() && descriptor.identity != filter.identity)
            continue;
        if (!filter.sourceId.isEmpty() && descriptor.sourceId != filter.sourceId)
            continue;
        if (!filter.category.isEmpty() && descriptor.category != filter.category)
            continue;
        if (filter.filterSource && descriptor.source != filter.source)
            continue;
        if (filter.filterStatus && descriptor.status != filter.status)
            continue;
        result.append(descriptor);
    }
    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        const QString leftKey = componentKey(left).toString();
        const QString rightKey = componentKey(right).toString();
        return leftKey < rightKey;
    });
    return result;
}

ComponentDescriptor ComponentCatalog::find(const QString &identity, const QString &version,
                                           const QString &sourceId) const
{
    QReadLocker locker(&m_lock);
    if (!sourceId.isEmpty())
        return m_entries.value({identity, sourceId, version}).descriptor;

    for (const auto &entry : m_entries)
    {
        if (entry.descriptor.identity == identity && entry.descriptor.version == version)
            return entry.descriptor;
    }
    return {};
}

QList<ComponentDiagnostic> ComponentCatalog::diagnostics(const ComponentKey &key) const
{
    QReadLocker locker(&m_lock);
    return m_entries.value(key).diagnostics;
}

quint64 ComponentCatalog::snapshotVersion() const
{
    QReadLocker locker(&m_lock);
    return m_snapshotVersion;
}

} // namespace cme::runtime
