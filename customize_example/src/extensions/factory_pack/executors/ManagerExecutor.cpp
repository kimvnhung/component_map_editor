#include "ManagerExecutor.h"

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"
#include "extensions/providers/executors/customizeexecutioncommon.h"

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
    const QVariantMap context = customize::executors::mergeIncomingTokens(incomingTokens);

    if (!context.contains("revenue") || !context.contains("request_buy"))
    {
        const QString msg = QStringLiteral("Missing required input tokens 'revenue' or 'request_buy' for ManagerExecutor.");
        return customize::executors::failExecution(componentType, componentId, context, {}, "", msg, outputPayload, trace,
                error);
    }

    QVariantMap out;

    bool revenueOk;
    double revenue = context.value("revenue", 0).toDouble(&revenueOk);
    QString requestBuy = context.value("request_buy", "").toString();

    if (!revenueOk || requestBuy.isEmpty())
    {
        const QString msg = QStringLiteral("Invalid input tokens 'revenue' or 'request_buy' for ManagerExecutor.");
        return customize::executors::failExecution(componentType, componentId, context, {}, "", msg, outputPayload, trace,
                error);
    }

    double currentCap = componentSnapshot.value("capital", 0).toDouble();

    if (currentCap < 0)
    {
        currentCap = 0;
    }

    out["capital"] = currentCap + revenue;

    if (currentCap + revenue >= componentSnapshot.value("buyAmount", 0).toDouble())
    {
        out["request_buy"] = requestBuy;
    }

    return true;
}