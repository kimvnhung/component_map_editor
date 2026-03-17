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
    int endCount   = 0;

    for (const QVariant &v : components) {
        const QVariantMap node = v.toMap();
        const QString id   = node.value(QStringLiteral("id")).toString();
        const QString type = node.value(QStringLiteral("type")).toString();
        if (!id.isEmpty())
            componentIds.insert(id);
        if (type == QStringLiteral("start"))
            ++startCount;
        if (type == QStringLiteral("end"))
            ++endCount;
    }

    // W001 – exactly one start node
    if (startCount != 1) {
        issues.append(QVariantMap{
            { QStringLiteral("code"),       QStringLiteral("W001") },
            { QStringLiteral("severity"),   QStringLiteral("error") },
            { QStringLiteral("message"),    QStringLiteral("Graph must contain exactly one start node (found %1).").arg(startCount) },
            { QStringLiteral("entityType"), QStringLiteral("graph") },
            { QStringLiteral("entityId"),   QString() }
        });
    }

    // W002 – exactly one end node
    if (endCount != 1) {
        issues.append(QVariantMap{
            { QStringLiteral("code"),       QStringLiteral("W002") },
            { QStringLiteral("severity"),   QStringLiteral("error") },
            { QStringLiteral("message"),    QStringLiteral("Graph must contain exactly one end node (found %1).").arg(endCount) },
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
                { QStringLiteral("message"),    QStringLiteral("Connection sourceId '%1' does not reference a known node.").arg(sourceId) },
                { QStringLiteral("entityType"), QStringLiteral("connection") },
                { QStringLiteral("entityId"),   connId }
            });
        }
        if (!componentIds.contains(targetId)) {
            issues.append(QVariantMap{
                { QStringLiteral("code"),       QStringLiteral("W003") },
                { QStringLiteral("severity"),   QStringLiteral("error") },
                { QStringLiteral("message"),    QStringLiteral("Connection targetId '%1' does not reference a known node.").arg(targetId) },
                { QStringLiteral("entityType"), QStringLiteral("connection") },
                { QStringLiteral("entityId"),   connId }
            });
        }
    }

    // W004 – no isolated nodes (no incoming and no outgoing connections)
    if (!components.isEmpty()) {
        QSet<QString> connectedIds;
        for (const QVariant &v : connections) {
            const QVariantMap conn = v.toMap();
            connectedIds.insert(conn.value(QStringLiteral("sourceId")).toString());
            connectedIds.insert(conn.value(QStringLiteral("targetId")).toString());
        }

        for (const QVariant &v : components) {
            const QVariantMap node = v.toMap();
            const QString id = node.value(QStringLiteral("id")).toString();
            if (!id.isEmpty() && !connectedIds.contains(id)) {
                issues.append(QVariantMap{
                    { QStringLiteral("code"),       QStringLiteral("W004") },
                    { QStringLiteral("severity"),   QStringLiteral("warning") },
                    { QStringLiteral("message"),    QStringLiteral("Node '%1' has no connections.").arg(id) },
                    { QStringLiteral("entityType"), QStringLiteral("node") },
                    { QStringLiteral("entityId"),   id }
                });
            }
        }
    }

    return issues;
}
