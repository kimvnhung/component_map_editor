#include "ManagerExecutor.h"

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"

QString ManagerExecutor::providerId() const
{
    return QStringLiteral("factory.execution.manager");
}

QStringList ManagerExecutor::supportedComponentTypes() const
{
    return { FactoryComponentTypeProvider::TypeManager };
}

QStringList ManagerExecutor::providedOutputKeys(const QString &componentType) const
{
    Q_UNUSED(componentType);
    return { QStringLiteral("request_buy") };
}

bool ManagerExecutor::executeComponent(const QString &componentType, const QString &componentId,
                                       const QVariantMap &componentSnapshot,
                                       const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload,
                                       QVariantMap *trace, QString *error) const
{
    Q_UNUSED(componentType);
    Q_UNUSED(componentId);
    Q_UNUSED(componentSnapshot);
    Q_UNUSED(incomingTokens);
    Q_UNUSED(trace);
    Q_UNUSED(error);

    if (outputPayload)
    {
        // For demonstration purposes, we simulate a manager that can request to buy fruit.
        (*outputPayload)[QStringLiteral("request_buy")] = QStringLiteral("store_1"); // Request buy from store_1
    }

    return true;
}