#include "StoreExecutor.h"

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"

QString StoreExecutor::providerId() const
{
    return QStringLiteral("factory.execution.store");
}

QStringList StoreExecutor::supportedComponentTypes() const
{
    return { FactoryComponentTypeProvider::TypeStore };
}

QStringList StoreExecutor::providedOutputKeys(const QString &componentType) const
{
    Q_UNUSED(componentType);
    return { QStringLiteral("wash"), QStringLiteral("fruit_type"),
             QStringLiteral("sell") };
}

bool StoreExecutor::executeComponent(const QString &componentType, const QString &componentId,
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
        // For demonstration purposes, we simulate a store that can wash and sell fruit.
        (*outputPayload)[QStringLiteral("wash")] = 5; // Wash 5 units of fruit
        (*outputPayload)[QStringLiteral("fruit_type")] = QStringLiteral("apple"); // Fruit type is apple
        (*outputPayload)[QStringLiteral("sell")] = 3; // Sell 3 units of fruit
    }

    return true;
}
