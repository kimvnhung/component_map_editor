#include "customizeconnectionpolicyprovider.h"

#include "customizecomponenttypeprovider.h"

QString CustomizeConnectionPolicyProvider::providerId() const
{
    return QStringLiteral("customize.workflow.connectionPolicy");
}

bool CustomizeConnectionPolicyProvider::canConnect(const QString &sourceTypeId,
                                                   const QString &targetTypeId,
                                                   const QVariantMap &context,
                                                   QString *reason) const
{
    const int targetIncomingCount =
        context.value(QStringLiteral("targetIncomingCount")).toInt();
    const int sourceOutgoingCount =
        context.value(QStringLiteral("sourceOutgoingCount")).toInt();

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

    // Accept only one connection to condition
    if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeCondition)) {
        if (targetIncomingCount >= 1) {
            if (reason)
                *reason = QStringLiteral("Condition component accepts only one incoming connection.");
            return false;
        }

        return true;
    }

    // Process only accept one outgoing connection and one incoming connection.
    if (sourceTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeProcess)) {
        if (sourceOutgoingCount >= 1) {
            if (reason)
                *reason = QStringLiteral("Process component accepts only one outgoing connection.");
            return false;
        }

        return true;
    }
    if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeProcess)) {
        if (targetIncomingCount >= 1) {
            if (reason)
                *reason = QStringLiteral("Process component accepts only one incoming connection.");
            return false;
        }

        return true;
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