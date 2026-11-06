#include "component_map_runtime/ComponentCatalog.h"

#include <QRegularExpression>
#include <algorithm>

namespace cme::runtime
{

namespace
{
int compareSemanticVersion(const QString &left, const QString &right)
{
    const QStringList leftParts = left.split(QLatin1Char('.'));
    const QStringList rightParts = right.split(QLatin1Char('.'));
    for (int i = 0; i < 3; ++i)
    {
        const int leftValue = i < leftParts.size()
            ? leftParts.at(i).section(QRegularExpression(QStringLiteral("[-+]")), 0, 0).toInt()
            : 0;
        const int rightValue = i < rightParts.size()
            ? rightParts.at(i).section(QRegularExpression(QStringLiteral("[-+]")), 0, 0).toInt()
            : 0;
        if (leftValue != rightValue)
            return leftValue < rightValue ? -1 : 1;
    }
    return 0;
}
}

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

QList<ComponentDescriptor> ComponentCatalog::snapshotForSource(const QString &sourceId) const
{
    CatalogFilter filter;
    filter.sourceId = sourceId.trimmed();
    return snapshot(filter);
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

ComponentDescriptor ComponentCatalog::resolve(const QString &identity, const QString &version,
                                              QString *error) const
{
    const QString normalizedIdentity = identity.trimmed();
    const QString normalizedVersion = version.trimmed();
    if (normalizedIdentity.isEmpty() || normalizedVersion.isEmpty())
    {
        if (error) *error = QStringLiteral("Component identity and version are required.");
        return {};
    }

    QReadLocker locker(&m_lock);
    ComponentDescriptor match;
    QString newestVersion;
    bool found = false;
    for (const auto &entry : m_entries)
    {
        const auto &descriptor = entry.descriptor;
        if (descriptor.identity != normalizedIdentity)
            continue;
        if (newestVersion.isEmpty() || compareSemanticVersion(newestVersion, descriptor.version) < 0)
            newestVersion = descriptor.version;
        if (descriptor.version == normalizedVersion)
        {
            if (!found)
            {
                match = descriptor;
                found = true;
            }
            else if (descriptor.sourceId != match.sourceId)
            {
                if (error) *error = QStringLiteral("Component identity/version resolves to multiple sources.");
                return {};
            }
        }
    }
    if (!found)
    {
        if (error)
        {
            *error = QStringLiteral("Component identity/version was not found.");
            if (!newestVersion.isEmpty() && compareSemanticVersion(normalizedVersion, newestVersion) < 0)
                *error += QStringLiteral(" A newer version (%1) exists; silent version substitution is not allowed.")
                               .arg(newestVersion);
        }
        return {};
    }
    return match;
}

QList<ComponentDiagnostic> ComponentCatalog::diagnostics(const ComponentKey &key) const
{
    QReadLocker locker(&m_lock);
    return m_entries.value(key).diagnostics;
}

void ComponentCatalog::recordDiagnostic(const ComponentKey &key, const ComponentDiagnostic &diagnostic)
{
    if (!key.isValid() || !diagnostic.isValid())
        return;
    QWriteLocker locker(&m_lock);
    auto entry = m_entries.find(key);
    if (entry == m_entries.end())
    {
        Entry newEntry;
        newEntry.diagnostics.append(diagnostic);
        m_entries.insert(key, std::move(newEntry));
        return;
    }
    entry->diagnostics.append(diagnostic);
}

QList<ComponentDiagnostic> ComponentCatalog::allDiagnostics() const
{
    QReadLocker locker(&m_lock);
    QList<ComponentDiagnostic> result;
    for (const auto &entry : m_entries)
        result.append(entry.diagnostics);
    return result;
}

quint64 ComponentCatalog::snapshotVersion() const
{
    QReadLocker locker(&m_lock);
    return m_snapshotVersion;
}

} // namespace cme::runtime
