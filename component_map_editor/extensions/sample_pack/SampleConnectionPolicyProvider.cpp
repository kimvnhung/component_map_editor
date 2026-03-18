#include "SampleConnectionPolicyProvider.h"

#include "SampleComponentTypeProvider.h"

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
    if (targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeStart)) {
        if (reason)
            *reason = QStringLiteral("Start component does not accept incoming connections.");
        return false;
    }

    // End node has no outgoing connections.
    if (sourceTypeId == QLatin1String(SampleComponentTypeProvider::TypeEnd)) {
        if (reason)
            *reason = QStringLiteral("End component does not have outgoing connections.");
        return false;
    }

    // start -> task only.
    if (sourceTypeId == QLatin1String(SampleComponentTypeProvider::TypeStart)) {
        if (targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeTask))
            return true;
        if (reason)
            *reason = QStringLiteral("Start component can only connect to task components.");
        return false;
    }

    // task -> task or decision.
    if (sourceTypeId == QLatin1String(SampleComponentTypeProvider::TypeTask)) {
        if (targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeTask) ||
            targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeDecision))
            return true;
        if (reason)
            *reason = QStringLiteral("Task component can connect to task or decision components only.");
        return false;
    }

    // decision -> task or end.
    if (sourceTypeId == QLatin1String(SampleComponentTypeProvider::TypeDecision)) {
        if (targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeTask) ||
            targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeEnd))
            return true;
        if (reason)
            *reason = QStringLiteral("Decision component can connect to task or end components only.");
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
