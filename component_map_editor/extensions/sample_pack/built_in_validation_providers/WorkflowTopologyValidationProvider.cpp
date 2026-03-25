#include "WorkflowTopologyValidationProvider.h"

#include <QHash>
#include <QQueue>
#include <QSet>

namespace {

QVariantMap makeIssue(const QString &code,
                      const QString &severity,
                      const QString &message,
                      const QString &entityType,
                      const QString &entityId)
{
    return QVariantMap{
        { QStringLiteral("code"), code },
        { QStringLiteral("severity"), severity },
        { QStringLiteral("message"), message },
        { QStringLiteral("entityType"), entityType },
        { QStringLiteral("entityId"), entityId }
    };
}

} // namespace

QString WorkflowTopologyValidationProvider::providerId() const
{
    return QStringLiteral("sample.workflow.validation.topology");
}

QVariantList WorkflowTopologyValidationProvider::validateGraph(const QVariantMap &graphSnapshot) const
{
    QVariantList issues;

    const QVariantList components  = graphSnapshot.value(QStringLiteral("components")).toList();
    const QVariantList connections = graphSnapshot.value(QStringLiteral("connections")).toList();

    QSet<QString> componentIds;
    QHash<QString, QString> componentTypeById;
    QHash<QString, int> incomingConnectionCount;
    QHash<QString, int> outgoingConnectionCount;
    QHash<QString, QStringList> adjacency;
    int startCount = 0;
    QString startId;

    for (const QVariant &v : components) {
        const QVariantMap component = v.toMap();
        const QString id   = component.value(QStringLiteral("id")).toString();
        const QString type = component.value(QStringLiteral("type")).toString();
        if (id.isEmpty())
            continue;

        componentIds.insert(id);
        componentTypeById.insert(id, type);
        incomingConnectionCount.insert(id, 0);
        outgoingConnectionCount.insert(id, 0);
        adjacency.insert(id, {});

        if (type == QStringLiteral("start")) {
            ++startCount;
            startId = id;
        }
    }

    for (const QVariant &v : connections) {
        const QVariantMap conn     = v.toMap();
        const QString sourceId     = conn.value(QStringLiteral("sourceId")).toString();
        const QString targetId     = conn.value(QStringLiteral("targetId")).toString();

        if (componentIds.contains(sourceId) && componentIds.contains(targetId) && sourceId != targetId) {
            outgoingConnectionCount[sourceId] = outgoingConnectionCount.value(sourceId, 0) + 1;
            incomingConnectionCount[targetId] = incomingConnectionCount.value(targetId, 0) + 1;
            adjacency[sourceId].append(targetId);
        }
    }

    if (!components.isEmpty()) {
        for (auto it = componentTypeById.constBegin(); it != componentTypeById.constEnd(); ++it) {
            const QString &id = it.key();
            const QString &type = it.value();
            const int incomingCount = incomingConnectionCount.value(id, 0);
            const int outgoingCount = outgoingConnectionCount.value(id, 0);

            if (incomingCount == 0 && outgoingCount == 0) {
                issues.append(makeIssue(QStringLiteral("W004"),
                                        QStringLiteral("warning"),
                                        QStringLiteral("Component '%1' has no connections.").arg(id),
                                        QStringLiteral("component"),
                                        id));
            }

            if (type == QStringLiteral("start")) {
                if (outgoingCount == 0) {
                    issues.append(makeIssue(QStringLiteral("W007"),
                                            QStringLiteral("error"),
                                            QStringLiteral("Start component '%1' must have at least one outgoing connection")
                                                .arg(id),
                                            QStringLiteral("component"),
                                            id));
                }
                continue;
            }

            if (type == QStringLiteral("stop")) {
                if (incomingCount == 0) {
                    issues.append(makeIssue(QStringLiteral("W007"),
                                            QStringLiteral("error"),
                                            QStringLiteral("Stop component '%1' must have at least one incoming connection")
                                                .arg(id),
                                            QStringLiteral("component"),
                                            id));
                }
                continue;
            }

            if (incomingCount == 0) {
                issues.append(makeIssue(QStringLiteral("W007"),
                                        QStringLiteral("error"),
                                        QStringLiteral("Component '%1' must have at least one incoming connection")
                                            .arg(id),
                                        QStringLiteral("component"),
                                        id));
            }
            if (outgoingCount == 0) {
                issues.append(makeIssue(QStringLiteral("W007"),
                                        QStringLiteral("error"),
                                        QStringLiteral("Component '%1' must have at least one outgoing connection")
                                            .arg(id),
                                        QStringLiteral("component"),
                                        id));
            }
        }

        if (startCount == 1 && componentIds.contains(startId)) {
            QSet<QString> visited;
            QQueue<QString> queue;
            visited.insert(startId);
            queue.enqueue(startId);

            while (!queue.isEmpty()) {
                const QString current = queue.dequeue();
                const QStringList targets = adjacency.value(current);
                for (const QString &targetId : targets) {
                    if (visited.contains(targetId))
                        continue;
                    visited.insert(targetId);
                    queue.enqueue(targetId);
                }
            }

            for (const QString &id : std::as_const(componentIds)) {
                if (!visited.contains(id)) {
                    issues.append(makeIssue(QStringLiteral("W008"),
                                            QStringLiteral("error"),
                                            QStringLiteral("Component '%1' is not reachable from start component '%2'")
                                                .arg(id, startId),
                                            QStringLiteral("component"),
                                            id));
                }
            }
        }
    }

    return issues;
}
