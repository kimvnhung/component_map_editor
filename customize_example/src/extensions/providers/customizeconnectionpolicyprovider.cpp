#include "customizeconnectionpolicyprovider.h"

#include "customizecomponenttypeprovider.h"

QString CustomizeConnectionPolicyProvider::providerId() const
{
    return QStringLiteral("customize.workflow.connectionPolicy");
}

bool CustomizeConnectionPolicyProvider::canConnect(const QString &sourceTypeId,
                                                const QString &targetTypeId,
                                                const QVariantMap & /*context*/,
                                                QString *reason) const
{
    // Nothing can connect TO start.
    if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeStart)) {
        if (reason)
            *reason = QStringLiteral("Start component does not accept incoming connections.");
        return false;
    }

    // Stop node has no outgoing connections.
    if (sourceTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeStop)) {
        if (reason)
            *reason = QStringLiteral("Stop component does not have outgoing connections.");
        return false;
    }

    // start -> process only.
    if (sourceTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeStart)) {
        if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeProcess))
            return true;
        if (reason)
            *reason = QStringLiteral("Start component can only connect to process components.");
        return false;
    }

    // process -> process or stop.
    if (sourceTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeProcess)) {
        if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeProcess) ||
            targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeStop))
            return true;
        if (reason)
            *reason = QStringLiteral("Process component can connect to process or stop components only.");
        return false;
    }

    if (reason)
        *reason = QStringLiteral("Unknown connection between types '%1' and '%2'.")
                      .arg(sourceTypeId, targetTypeId);
    return false;
}

QVariantMap CustomizeConnectionPolicyProvider::normalizeConnectionProperties(
    const QString & /*sourceTypeId*/,
    const QString & /*targetTypeId*/,
    const QVariantMap &rawProperties) const
{
    QVariantMap result = rawProperties;
    if (!result.contains(QStringLiteral("type")))
        result.insert(QStringLiteral("type"), QStringLiteral("flow"));
    return result;
}