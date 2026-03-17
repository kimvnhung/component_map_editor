#include "SampleConnectionPolicyProvider.h"

#include "SampleNodeTypeProvider.h"

QString SampleConnectionPolicyProvider::providerId() const
{
    return QStringLiteral("sample.workflow.connectionPolicy");
}

bool SampleConnectionPolicyProvider::canConnect(const QString &sourceTypeId,
                                                const QString &targetTypeId,
                                                const QVariantMap & /*context*/,
                                                QString *reason) const
{
    // Nothing can connect TO start.
    if (targetTypeId == QLatin1String(SampleNodeTypeProvider::TypeStart)) {
        if (reason)
            *reason = QStringLiteral("Start node does not accept incoming connections.");
        return false;
    }

    // End node has no outgoing connections.
    if (sourceTypeId == QLatin1String(SampleNodeTypeProvider::TypeEnd)) {
        if (reason)
            *reason = QStringLiteral("End node does not have outgoing connections.");
        return false;
    }

    // start -> task only.
    if (sourceTypeId == QLatin1String(SampleNodeTypeProvider::TypeStart)) {
        if (targetTypeId == QLatin1String(SampleNodeTypeProvider::TypeTask))
            return true;
        if (reason)
            *reason = QStringLiteral("Start node can only connect to task nodes.");
        return false;
    }

    // task -> task or decision.
    if (sourceTypeId == QLatin1String(SampleNodeTypeProvider::TypeTask)) {
        if (targetTypeId == QLatin1String(SampleNodeTypeProvider::TypeTask) ||
            targetTypeId == QLatin1String(SampleNodeTypeProvider::TypeDecision))
            return true;
        if (reason)
            *reason = QStringLiteral("Task node can connect to task or decision nodes only.");
        return false;
    }

    // decision -> task or end.
    if (sourceTypeId == QLatin1String(SampleNodeTypeProvider::TypeDecision)) {
        if (targetTypeId == QLatin1String(SampleNodeTypeProvider::TypeTask) ||
            targetTypeId == QLatin1String(SampleNodeTypeProvider::TypeEnd))
            return true;
        if (reason)
            *reason = QStringLiteral("Decision node can connect to task or end nodes only.");
        return false;
    }

    if (reason)
        *reason = QStringLiteral("Unknown connection between types '%1' and '%2'.")
                      .arg(sourceTypeId, targetTypeId);
    return false;
}

QVariantMap SampleConnectionPolicyProvider::normalizeConnectionProperties(
    const QString & /*sourceTypeId*/,
    const QString & /*targetTypeId*/,
    const QVariantMap &rawProperties) const
{
    QVariantMap result = rawProperties;
    if (!result.contains(QStringLiteral("type")))
        result.insert(QStringLiteral("type"), QStringLiteral("flow"));
    return result;
}
