#include "WorkflowConnectionValidationProvider.h"

#include "models/ConnectionModel.h"

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

bool validSideValue(int sideValue)
{
    return sideValue == static_cast<int>(ConnectionModel::SideAuto)
           || sideValue == static_cast<int>(ConnectionModel::SideTop)
           || sideValue == static_cast<int>(ConnectionModel::SideRight)
           || sideValue == static_cast<int>(ConnectionModel::SideBottom)
           || sideValue == static_cast<int>(ConnectionModel::SideLeft);
}

} // namespace

QString WorkflowConnectionValidationProvider::providerId() const
{
    return QStringLiteral("sample.workflow.validation.connection");
}

QVariantList WorkflowConnectionValidationProvider::validateGraph(const QVariantMap &graphSnapshot) const
{
    QVariantList issues;

    const QVariantList components  = graphSnapshot.value(QStringLiteral("components")).toList();
    const QVariantList connections = graphSnapshot.value(QStringLiteral("connections")).toList();

    QSet<QString> componentIds;
    for (const QVariant &v : components) {
        const QString id = v.toMap().value(QStringLiteral("id")).toString();
        if (!id.isEmpty())
            componentIds.insert(id);
    }

    QSet<QString> connectionIds;
    for (const QVariant &v : connections) {
        const QVariantMap conn     = v.toMap();
        const QString connId       = conn.value(QStringLiteral("id")).toString();
        const QString sourceId     = conn.value(QStringLiteral("sourceId")).toString();
        const QString targetId     = conn.value(QStringLiteral("targetId")).toString();
        const QString connLabel    = conn.value(QStringLiteral("label")).toString();
        const int sourceSide       = conn.value(QStringLiteral("sourceSide")).toInt();
        const int targetSide       = conn.value(QStringLiteral("targetSide")).toInt();

        if (connId.isEmpty()) {
            issues.append(makeIssue(QStringLiteral("W006"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Connection has an empty id"),
                                    QStringLiteral("connection"),
                                    QString()));
        } else if (connectionIds.contains(connId)) {
            issues.append(makeIssue(QStringLiteral("W006"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Duplicate connection id: %1").arg(connId),
                                    QStringLiteral("connection"),
                                    connId));
        } else {
            connectionIds.insert(connId);
        }

        if (!componentIds.contains(sourceId)) {
            issues.append(makeIssue(QStringLiteral("W003"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Connection '%1' references unknown source component '%2'")
                                        .arg(connId, sourceId),
                                    QStringLiteral("connection"),
                                    connId));
        }
        if (!componentIds.contains(targetId)) {
            issues.append(makeIssue(QStringLiteral("W003"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Connection '%1' references unknown target component '%2'")
                                        .arg(connId, targetId),
                                    QStringLiteral("connection"),
                                    connId));
        }

        if (sourceId == targetId && !sourceId.isEmpty()) {
            issues.append(makeIssue(QStringLiteral("W006"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Connection '%1' is a self-loop").arg(connId),
                                    QStringLiteral("connection"),
                                    connId));
        }

        if (!validSideValue(sourceSide)) {
            issues.append(makeIssue(QStringLiteral("W006"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Connection '%1' (%2 -> %3, label '%4') has invalid sourceSide value %5. "
                                                   "Expected one of: Auto(-1), Top(0), Right(1), Bottom(2), Left(3).")
                                        .arg(connId, sourceId, targetId, connLabel)
                                        .arg(sourceSide),
                                    QStringLiteral("connection"),
                                    connId));
        }

        if (!validSideValue(targetSide)) {
            issues.append(makeIssue(QStringLiteral("W006"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Connection '%1' (%2 -> %3, label '%4') has invalid targetSide value %5. "
                                                   "Expected one of: Auto(-1), Top(0), Right(1), Bottom(2), Left(3).")
                                        .arg(connId, sourceId, targetId, connLabel)
                                        .arg(targetSide),
                                    QStringLiteral("connection"),
                                    connId));
        }
    }

    return issues;
}
