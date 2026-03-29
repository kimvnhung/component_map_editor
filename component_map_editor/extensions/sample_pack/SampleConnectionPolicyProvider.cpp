#include "SampleConnectionPolicyProvider.h"

#include "SampleComponentTypeProvider.h"

QString SampleConnectionPolicyProvider::providerId() const
{
    return QStringLiteral("sample.workflow.connectionPolicy");
}

bool SampleConnectionPolicyProvider::canConnect(const cme::ConnectionPolicyContext &context,
                                                QString *reason) const
{
    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());

    // Nothing can connect TO start.
    if (targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeStart)) {
        if (reason)
            *reason = QStringLiteral("Start component does not accept incoming connections.");
        return false;
    }

    // Stop node has no outgoing connections.
    if (sourceTypeId == QLatin1String(SampleComponentTypeProvider::TypeStop)) {
        if (reason)
            *reason = QStringLiteral("Stop component does not have outgoing connections.");
        return false;
    }

    // start -> process only.
    if (sourceTypeId == QLatin1String(SampleComponentTypeProvider::TypeStart)) {
        if (targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeProcess))
            return true;
        if (reason)
            *reason = QStringLiteral("Start component can only connect to process components.");
        return false;
    }

    // process -> process or stop.
    if (sourceTypeId == QLatin1String(SampleComponentTypeProvider::TypeProcess)) {
        if (targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeProcess)
            || targetTypeId == QLatin1String(SampleComponentTypeProvider::TypeStop)) {
            return true;
        }
        if (reason)
            *reason = QStringLiteral("Process component can connect to process or stop components only.");
        return false;
    }

    if (reason)
        *reason = QStringLiteral("Unknown connection between types '%1' and '%2'.")
                      .arg(sourceTypeId, targetTypeId);
    return false;
}

QVariantMap SampleConnectionPolicyProvider::normalizeConnectionProperties(
    const cme::ConnectionPolicyContext & /*context*/,
    const QVariantMap &rawProperties) const
{
    QVariantMap result = rawProperties;
    if (!result.contains(QStringLiteral("type")))
        result.insert(QStringLiteral("type"), QStringLiteral("flow"));
    return result;
}
