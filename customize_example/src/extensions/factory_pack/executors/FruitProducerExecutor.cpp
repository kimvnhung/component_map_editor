#include "FruitProducerExecutor.h"

#include <base_log.h>

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"
#include "extensions/providers/executors/customizeexecutioncommon.h"

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
    Q_UNUSED(incomingTokens);
    Q_UNUSED(trace);
    Q_UNUSED(error);

    const QVariantMap context = customize::executors::mergeIncomingTokens(incomingTokens);
    QVariantMap out;

    if (!context.contains("buy") || !context.contains("producer_id"))
    {
        const QString msg = QStringLiteral("Missing required input tokens 'buy' or 'producer_id' for FruitProducerExecutor.");
        return customize::executors::failExecution(componentType, componentId, context, out, "", msg, outputPayload, trace,
                error);
    }

    if (outputPayload)
    {
        *outputPayload = out;
    }

    if (trace)
    {
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);
    }

    LOGIF("FruitProducerExecutor executing component: {}", componentId.toStdString());
    return true;
}
