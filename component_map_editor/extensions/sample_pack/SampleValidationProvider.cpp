#include "SampleValidationProvider.h"

#include <QSet>

QString SampleValidationProvider::providerId() const
{
    return QStringLiteral("sample.workflow.validation");
}

QVariantList SampleValidationProvider::validateGraph(const QVariantMap &graphSnapshot) const
{
    QVariantList issues;

    const QVariantList components  = graphSnapshot.value(QStringLiteral("components")).toList();
    const QVariantList connections = graphSnapshot.value(QStringLiteral("connections")).toList();

    QSet<QString> componentIds;
    int startCount = 0;
    int stopCount  = 0;

    for (const QVariant &v : components) {
        const QVariantMap component = v.toMap();
        const QString id   = component.value(QStringLiteral("id")).toString();
        const QString type = component.value(QStringLiteral("type")).toString();
        if (!id.isEmpty())
            componentIds.insert(id);
        if (type == QStringLiteral("start"))
            ++startCount;
        if (type == QStringLiteral("stop"))
            ++stopCount;
    }

    // W001 – exactly one start component
    if (startCount != 1) {
        issues.append(QVariantMap{
            { QStringLiteral("code"),       QStringLiteral("W001") },
            { QStringLiteral("severity"),   QStringLiteral("error") },
            { QStringLiteral("message"),    QStringLiteral("Graph must contain exactly one start component (found %1).").arg(startCount) },
            { QStringLiteral("entityType"), QStringLiteral("graph") },
            { QStringLiteral("entityId"),   QString() }
        });
    }

    // W002 – exactly one stop component
    if (stopCount != 1) {
        issues.append(QVariantMap{
            { QStringLiteral("code"),       QStringLiteral("W002") },
            { QStringLiteral("severity"),   QStringLiteral("error") },
            { QStringLiteral("message"),    QStringLiteral("Graph must contain exactly one stop component (found %1).").arg(stopCount) },
            { QStringLiteral("entityType"), QStringLiteral("graph") },
            { QStringLiteral("entityId"),   QString() }
        });
    }

    // W003 – all connection endpoints reference known component ids
    for (const QVariant &v : connections) {
        const QVariantMap conn     = v.toMap();
        const QString connId       = conn.value(QStringLiteral("id")).toString();
        const QString sourceId     = conn.value(QStringLiteral("sourceId")).toString();
        const QString targetId     = conn.value(QStringLiteral("targetId")).toString();

        if (!componentIds.contains(sourceId)) {
            issues.append(QVariantMap{
                { QStringLiteral("code"),       QStringLiteral("W003") },
                { QStringLiteral("severity"),   QStringLiteral("error") },
                { QStringLiteral("message"),    QStringLiteral("Connection sourceId '%1' does not reference a known component.").arg(sourceId) },
                { QStringLiteral("entityType"), QStringLiteral("connection") },
                { QStringLiteral("entityId"),   connId }
            });
        }
        if (!componentIds.contains(targetId)) {
            issues.append(QVariantMap{
                { QStringLiteral("code"),       QStringLiteral("W003") },
                { QStringLiteral("severity"),   QStringLiteral("error") },
                { QStringLiteral("message"),    QStringLiteral("Connection targetId '%1' does not reference a known component.").arg(targetId) },
                { QStringLiteral("entityType"), QStringLiteral("connection") },
                { QStringLiteral("entityId"),   connId }
            });
        }
    }

    // W004 – no isolated components (no incoming and no outgoing connections)
    if (!components.isEmpty()) {
        QSet<QString> connectedIds;
        for (const QVariant &v : connections) {
            const QVariantMap conn = v.toMap();
            connectedIds.insert(conn.value(QStringLiteral("sourceId")).toString());
            connectedIds.insert(conn.value(QStringLiteral("targetId")).toString());
        }

        for (const QVariant &v : components) {
            const QVariantMap component = v.toMap();
            const QString id = component.value(QStringLiteral("id")).toString();
            if (!id.isEmpty() && !connectedIds.contains(id)) {
                issues.append(QVariantMap{
                    { QStringLiteral("code"),       QStringLiteral("W004") },
                    { QStringLiteral("severity"),   QStringLiteral("warning") },
                    { QStringLiteral("message"),    QStringLiteral("Component '%1' has no connections.").arg(id) },
                    { QStringLiteral("entityType"), QStringLiteral("component") },
                    { QStringLiteral("entityId"),   id }
                });
            }
        }
    }

    return issues;
}
