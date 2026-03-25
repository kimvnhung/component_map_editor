#include "WorkflowStructureValidationProvider.h"

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

QString WorkflowStructureValidationProvider::providerId() const
{
    return QStringLiteral("sample.workflow.validation.structure");
}

QVariantList WorkflowStructureValidationProvider::validateGraph(const QVariantMap &graphSnapshot) const
{
    QVariantList issues;

    const QVariantList components = graphSnapshot.value(QStringLiteral("components")).toList();

    QSet<QString> componentIds;
    int startCount = 0;
    int stopCount  = 0;

    for (const QVariant &v : components) {
        const QVariantMap component = v.toMap();
        const QString id   = component.value(QStringLiteral("id")).toString();
        const QString type = component.value(QStringLiteral("type")).toString();
        if (id.isEmpty()) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Component has an empty id"),
                                    QStringLiteral("component"),
                                    QString()));
            continue;
        }

        if (componentIds.contains(id)) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Duplicate component id: %1").arg(id),
                                    QStringLiteral("component"),
                                    id));
        } else {
            componentIds.insert(id);
        }

        if (type == QStringLiteral("start"))
            ++startCount;
        if (type == QStringLiteral("stop"))
            ++stopCount;

        if (component.value(QStringLiteral("width")).toDouble() <= 0.0) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Component '%1' has non-positive width").arg(id),
                                    QStringLiteral("component"),
                                    id));
        }
        if (component.value(QStringLiteral("height")).toDouble() <= 0.0) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Component '%1' has non-positive height").arg(id),
                                    QStringLiteral("component"),
                                    id));
        }
    }

    if (startCount != 1) {
        issues.append(makeIssue(QStringLiteral("W001"),
                                QStringLiteral("error"),
                                QStringLiteral("Graph must contain exactly one start component (found %1).").arg(startCount),
                                QStringLiteral("graph"),
                                QString()));
    }

    if (stopCount != 1) {
        issues.append(makeIssue(QStringLiteral("W002"),
                                QStringLiteral("error"),
                                QStringLiteral("Graph must contain exactly one stop component (found %1).").arg(stopCount),
                                QStringLiteral("graph"),
                                QString()));
    }

    return issues;
}
