#include "customizeconnectionpolicyprovider.h"

#include "customizecomponenttypeprovider.h"

QString CustomizeConnectionPolicyProvider::providerId() const
{
    return QStringLiteral("customize.workflow.connectionPolicy");
}

bool CustomizeConnectionPolicyProvider::canConnect(const cme::ConnectionPolicyContext &context,
                                                   QString *reason) const
{
    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());
    const int targetIncomingCount = context.target_incoming_count();
    const int sourceOutgoingCount = context.source_outgoing_count();

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

    // Accept only one connection to condition.
    if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeCondition)) {
        if (targetIncomingCount >= 1) {
            if (reason)
                *reason = QStringLiteral("Condition component accepts only one incoming connection.");
            return false;
        }

        return true;
    }

    // Process accepts one outgoing and one incoming connection.
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
    const cme::ConnectionPolicyContext & /*context*/,
    const QVariantMap &rawProperties) const
{
    QVariantMap result = rawProperties;
    if (!result.contains(QStringLiteral("type")))
        result.insert(QStringLiteral("type"), QStringLiteral("flow"));
    return result;
}