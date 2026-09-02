#include "FruitProducerExecutor.h"

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"

QString FruitProducerExecutor::providerId() const
{
    return QStringLiteral("factory.execution.fruit_producer");
}

QStringList FruitProducerExecutor::supportedComponentTypes() const
{
    return { FactoryComponentTypeProvider::TypeFruitProducer };
}

QStringList FruitProducerExecutor::providedOutputKeys(const QString &componentType) const
{
    Q_UNUSED(componentType);
    return { QStringLiteral("produced"), QStringLiteral("fruit_type"), QStringLiteral("request_buy") };
}

bool FruitProducerExecutor::executeComponent(const QString &componentType, const QString &componentId,
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
        // For demonstration purposes, we produce a fixed amount of fruit and a fixed fruit type.
        (*outputPayload)[QStringLiteral("produced")] = 10; // Produce 10 units of fruit
        (*outputPayload)[QStringLiteral("fruit_type")] = QStringLiteral("apple"); // Produce apples
        (*outputPayload)[QStringLiteral("request_buy")] = QStringLiteral("manager_1"); // Request buy from manager_1
    }

    return true;
}
